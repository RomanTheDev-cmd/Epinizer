// Epinizer 3 character specification (Claude, 2026-10-28), from Roman's remarks (Замечания к звуку и ошибкам.md) and
// «План следующей версии.md»: each character transforms the material itself with its own chain of effects; no octave
// layers, sub, harmonizer or doubled notes; the character survives a short tail with Echo off; Hold keeps a living
// texture instead of a short loop; the wet level is comparable with the dry one.
#include "EpicDSP.h"
#include <array>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr double sr = 48000.0, twoPi = 6.283185307179586;
int failures = 0, checks = 0;
void require (bool ok, const std::string& what)
{
    ++checks;
    if (! ok) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}
const char* names[] { "Glass", "Massive", "Horroble", "Warm" };
double db (double v) { return 10.0 * std::log10 (std::max (v, 1.0e-30)); } // of a power

struct Stereo { std::vector<float> l, r; };
Stereo render (int character, epinizer::Settings s, Stereo in)
{
    epinizer::Engine e;
    e.prepare (sr);
    s.character = character;
    e.snap (s);
    e.process (in.l.data(), in.r.data(), (int) in.l.size(), s);
    return in;
}
epinizer::Settings settings (float m1, float m2, float m3, bool echo = false)
{
    // m1 = density (Prism / Weight / Dread / Glow), m2 = space (Bloom / Scale / Abyss / Trail), m3 = motion.
    epinizer::Settings s;
    s.density = m1; s.space = m2; s.motion = m3; s.mix = 1.0f; s.echo = echo; s.hold = false; s.width = 1.0f;
    return s;
}

// Sources
Stereo sine (double hz, double amp, double seconds)
{
    Stereo s { std::vector<float> ((size_t) (seconds * sr)), {} };
    for (size_t n = 0; n < s.l.size(); ++n) s.l[n] = (float) (amp * std::sin (twoPi * hz * (double) n / sr));
    s.r = s.l;
    return s;
}
// Harmonic plucks (330 Hz, 6 harmonics), one every 0.6 s, 0.25 s decay.
Stereo plucks (double seconds, double hz = 330.0)
{
    Stereo s { std::vector<float> ((size_t) (seconds * sr)), {} };
    for (size_t n = 0; n < s.l.size(); ++n)
    {
        const double t = (double) n / sr, tp = std::fmod (t, 0.6);
        double x = 0.0;
        for (int k = 1; k <= 6; ++k) x += std::sin (twoPi * hz * k * tp) / k;
        s.l[n] = (float) (0.35 * x * std::exp (-tp / 0.25) * std::min (1.0, tp / 0.002));
    }
    s.r = s.l;
    return s;
}
// A drum loop: pitched kick and noise hats, 130 BPM.
Stereo drums (double seconds)
{
    Stereo s { std::vector<float> ((size_t) (seconds * sr)), std::vector<float> ((size_t) (seconds * sr)) };
    uint32_t seed = 12345u;
    const double beat = 60.0 / 130.0;
    for (size_t n = 0; n < s.l.size(); ++n)
    {
        const double t = (double) n / sr, tb = std::fmod (t, beat), th = std::fmod (t, beat / 2.0);
        seed = seed * 1664525u + 1013904223u;
        const double noise = (double) (seed >> 8) / 8388608.0 - 1.0;
        const double kick = 0.7 * std::exp (-tb / 0.15) * std::sin (twoPi * (45.0 * tb + 3.0 * (1.0 - std::exp (-tb / 0.03))));
        const double hat = 0.18 * std::exp (-th / 0.03) * noise;
        s.l[n] = (float) (kick + hat);
        s.r[n] = (float) (kick + 0.8 * hat);
    }
    return s;
}

double tone (const std::vector<float>& x, double hz, size_t from, size_t to) // power of one frequency (Goertzel)
{
    std::complex<double> acc {};
    for (size_t n = from; n < to; ++n) acc += (double) x[n] * std::polar (1.0, -twoPi * hz * (double) n / sr);
    const double a = std::abs (acc) / (double) (to - from);
    return a * a;
}
double power (const std::vector<float>& x, size_t from, size_t to)
{
    double e = 0.0;
    for (size_t n = from; n < to; ++n) e += (double) x[n] * x[n];
    return e / (double) std::max<size_t> (1, to - from);
}
// One-pole band split for features (deterministic, independent of the engine).
std::vector<float> band (const std::vector<float>& x, double lo, double hi)
{
    std::vector<float> y (x.size());
    double l1 = 0, l2 = 0, h1 = 0, h2 = 0;
    const double al = 1.0 - std::exp (-twoPi * hi / sr), ah = 1.0 - std::exp (-twoPi * lo / sr);
    for (size_t n = 0; n < x.size(); ++n)
    {
        l1 += al * (x[n] - l1); l2 += al * (l1 - l2);          // low-pass at hi
        h1 += ah * (l2 - h1); h2 += ah * (h1 - h2);            // minus a low-pass at lo
        y[n] = (float) (l2 - h2);
    }
    return y;
}

// Character features: octave band balance (dB rel. total), crest factor, sustain after attacks, side/mid.
std::vector<double> features (const Stereo& s, double period)
{
    std::vector<double> f;
    std::vector<float> mid (s.l.size()), side (s.l.size());
    for (size_t n = 0; n < mid.size(); ++n) { mid[n] = 0.5f * (s.l[n] + s.r[n]); side[n] = 0.5f * (s.l[n] - s.r[n]); }
    const size_t from = (size_t) (0.5 * sr), to = mid.size();
    const double total = power (mid, from, to);
    for (double c : { 90.0, 180.0, 360.0, 720.0, 1440.0, 2880.0, 5760.0, 11520.0 })
        f.push_back (db (power (band (mid, c / 1.41, c * 1.41), from, to) / total));
    double peak = 0.0;
    for (size_t n = from; n < to; ++n) peak = std::max (peak, (double) std::abs (mid[n]));
    f.push_back (db (peak * peak / total));
    // sustain: energy 150..450 ms after each period start vs 0..80 ms
    double early = 0.0, late = 0.0;
    for (double t0 = 0.6; t0 + period <= (double) to / sr; t0 += period)
    {
        early += power (mid, (size_t) (t0 * sr), (size_t) ((t0 + 0.08) * sr));
        late += power (mid, (size_t) ((t0 + 0.15) * sr), (size_t) ((t0 + 0.45) * sr));
    }
    f.push_back (db (late / early));
    f.push_back (std::max (-30.0, db (power (side, from, to) / total)));
    return f;
}
// How strongly the 10 ms envelope repeats with a period of 30..500 ms (the old Hold looped 280 ms). The slow course
// (150 ms moving average) is removed first: a texture that slowly evolves is not a loop.
double repetition (const std::vector<float>& x, size_t from)
{
    std::vector<double> env, hp;
    for (size_t n = from; n + 480 <= x.size(); n += 480) env.push_back (std::sqrt (power (x, n, n + 480)));
    for (size_t i = 0; i < env.size(); ++i)
    {
        double m = 0.0; int k = 0;
        for (size_t j = i >= 7 ? i - 7 : 0; j <= std::min (env.size() - 1, i + 7); ++j) { m += env[j]; ++k; }
        hp.push_back (env[i] - m / k);
    }
    double var = 0.0, worst = 0.0;
    for (double v : hp) var += v * v;
    for (size_t lag = 3; lag <= 50; ++lag)
    {
        double acc = 0.0;
        for (size_t i = lag; i < hp.size(); ++i) acc += hp[i] * hp[i - lag];
        worst = std::max (worst, acc / std::max (var, 1.0e-30));
    }
    return worst;
}
double distance (const std::vector<double>& a, const std::vector<double>& b)
{
    double d = 0.0;
    for (size_t i = 0; i < a.size(); ++i) d += std::abs (a[i] - b[i]);
    return d / (double) a.size();
}
} // namespace

int main()
{
    // 1. No extra notes: a held 220 Hz tone gets no sub-octave, no fifth and no octave layer.
    for (int c = 0; c < 4; ++c)
    {
        const auto out = render (c, settings (0.6f, 0.6f, 0.6f), sine (220.0, 0.3, 3.0));
        const size_t a = (size_t) sr, b = (size_t) (3.0 * sr);
        const double f0 = tone (out.l, 220.0, a, b);
        auto rel = [&] (double hz) { double m = 0.0; for (double d = -3.0; d <= 3.0; d += 1.0) m = std::max (m, tone (out.l, hz + d, a, b)); return db (m / f0); };
        require (rel (110.0) < -45.0 && rel (55.0) < -45.0 && rel (73.3) < -45.0,
                 std::string (names[c]) + ": no sub-octave notes (110 Hz " + std::to_string (rel (110.0)) + " dB)");
        require (rel (330.0) < -40.0, std::string (names[c]) + ": no fifth / harmonizer (" + std::to_string (rel (330.0)) + " dB)");
        require (rel (440.0) < -18.0, std::string (names[c]) + ": no octave layer (440 Hz " + std::to_string (rel (440.0)) + " dB)");
    }
    // 2. No sub build-up on bass material (Warm and Massive were heard adding sub).
    for (int c : { 1, 3 })
    {
        auto in = sine (55.0, 0.4, 4.0);
        for (size_t n = 0; n < in.l.size(); ++n) in.l[n] += (float) (0.15 * std::sin (twoPi * 110.0 * (double) n / sr));
        in.r = in.l;
        const auto out = render (c, settings (0.8f, 0.6f, 0.6f), in);
        const auto li = band (in.l, 20.0, 70.0), lo = band (out.l, 20.0, 70.0);
        const double d = db (power (lo, (size_t) sr, lo.size()) / power (li, (size_t) sr, li.size()));
        require (d < 1.5, std::string (names[c]) + ": no sub build-up on bass (" + std::to_string (d) + " dB below 70 Hz)");
    }
    // 3. The character lives in the material: shortest tail, Echo off - transformed, and all four distinct.
    for (int src = 0; src < 2; ++src)
    {
        const auto in = src == 0 ? plucks (6.0) : drums (6.0);
        const double period = src == 0 ? 0.6 : 60.0 / 130.0;
        const auto fin = features (in, period);
        std::array<std::vector<double>, 4> fo;
        for (int c = 0; c < 4; ++c)
        {
            fo[(size_t) c] = features (render (c, settings (0.6f, 0.0f, 0.6f), in), period);
            if (std::getenv ("EPINIZER_DEBUG")) { std::cerr << names[c] << (src ? " drums:" : " plucks:"); for (double v : fo[(size_t) c]) std::cerr << ' ' << (int) std::lround (v * 10) / 10.0; std::cerr << '\n'; }
            const double d = distance (fo[(size_t) c], fin);
            require (d >= 2.0, std::string (names[c]) + (src ? " on drums" : " on plucks") + ": the material itself changes with a short tail and Echo off (distance "
                     + std::to_string (d) + ")");
        }
        for (int a = 0; a < 4; ++a)
            for (int b = a + 1; b < 4; ++b)
            {
                const double d = distance (fo[(size_t) a], fo[(size_t) b]);
                require (d >= 1.5, std::string (names[a]) + " vs " + names[b] + (src ? " on drums" : " on plucks") + ": distinct characters (distance "
                         + std::to_string (d) + ")");
            }
    }
    // 4. Signatures.
    {
        // Warm: softer peaks and a softer top; drift moves the pitch of the processed sound slightly.
        const auto in = plucks (4.0);
        const auto out = render (3, settings (0.75f, 0.2f, 0.4f), in);
        const auto fi = features (in, 0.6), fw = features (out, 0.6);
        require (fw[8] <= fi[8] - 1.5, "Warm: softer peaks (crest " + std::to_string (fw[8]) + " vs " + std::to_string (fi[8]) + " dB)");
        require (fw[6] + fw[7] <= fi[6] + fi[7] - 6.0, "Warm: softer top (5.8 + 11.5 kHz bands " + std::to_string (fw[6] + fw[7]) + " vs "
                 + std::to_string (fi[6] + fi[7]) + " dB)");
        auto wobble = [] (const std::vector<float>& x) // std of the period of a 440 Hz tone, in cents
        {
            std::vector<double> zc;
            for (size_t n = (size_t) sr; n + 1 < x.size(); ++n)
                if (x[n] <= 0.0f && x[n + 1] > 0.0f) zc.push_back ((double) n + x[n] / (x[n] - x[n + 1]));
            std::vector<double> per;
            for (size_t i = 1; i < zc.size(); ++i) per.push_back (zc[i] - zc[i - 1]);
            double m = 0.0, v = 0.0;
            for (double p : per) m += p;
            m /= (double) per.size();
            for (double p : per) v += (1200.0 * std::log2 (p / m)) * (1200.0 * std::log2 (p / m));
            return std::sqrt (v / (double) per.size());
        };
        const double still = wobble (render (3, settings (0.0f, 0.0f, 0.0f), sine (440.0, 0.2, 4.0)).l);
        const double moving = wobble (render (3, settings (0.0f, 0.0f, 1.0f), sine (440.0, 0.2, 4.0)).l);
        require (moving >= 3.0 && moving > still * 2.0, "Warm: Drift moves the pitch of the processed sound (" + std::to_string (still) + " -> "
                 + std::to_string (moving) + " cents)");
    }
    {
        // Horroble: inharmonic (frequency-shifted) energy and a reversed swell after an isolated attack.
        const auto in = plucks (4.0);
        const auto out = render (2, settings (0.7f, 0.3f, 0.4f), in);
        double harmonic = 0.0, all = 0.0;
        for (double hz = 100.0; hz < 2400.0; hz += 2.0)
        {
            const double p = tone (out.l, hz, (size_t) sr, (size_t) (3.0 * sr));
            all += p;
            const double k = std::round (hz / 330.0);
            if (k >= 1.0 && std::abs (hz - k * 330.0) <= 6.0) harmonic += p;
        }
        require (1.0 - harmonic / all >= 0.15, "Horroble: atonal shift of the spectrum (" + std::to_string (100.0 * (1.0 - harmonic / all)) + " % off the harmonics)");
        Stereo one { std::vector<float> ((size_t) (2.0 * sr)), {} };
        for (size_t n = 0; n < (size_t) (0.4 * sr); ++n)
        {
            const double t = (double) n / sr;
            double x = 0.0;
            for (int k = 1; k <= 6; ++k) x += std::sin (twoPi * 330.0 * k * t) / k;
            one.l[n + (size_t) (0.2 * sr)] = (float) (0.35 * x * std::exp (-t / 0.08));
        }
        one.r = one.l;
        const auto o = render (2, settings (0.8f, 0.5f, 0.2f), one);
        const double at100 = power (o.l, (size_t) (0.29 * sr), (size_t) (0.31 * sr));
        double later = 0.0;
        for (double t = 0.4; t < 1.2; t += 0.02) later = std::max (later, power (o.l, (size_t) (t * sr), (size_t) ((t + 0.02) * sr)));
        require (later > at100, "Horroble: a reversed swell rises after the attack has decayed (" + std::to_string (db (later / at100)) + " dB)");
    }
    {
        // Glass: dispersion delays the highs against the body, and the bright bands open after the attack.
        Stereo imp { std::vector<float> ((size_t) sr), {} };
        imp.l[100] = 0.8f;
        imp.r = imp.l;
        const auto o = render (0, settings (0.8f, 0.0f, 0.0f), imp);
        auto centroid = [&] (double lo, double hi)
        {
            const auto b = band (o.l, lo, hi);
            double e = 0.0, et = 0.0;
            for (size_t n = 0; n < (size_t) (0.05 * sr); ++n) { e += (double) b[n] * b[n]; et += (double) b[n] * b[n] * (double) n; }
            return et / e / sr * 1000.0;
        };
        const double dt = centroid (4000.0, 8000.0) - centroid (300.0, 600.0);
        require (std::abs (dt) >= 1.5, "Glass: frequency-dependent dispersion (highs vs body " + std::to_string (dt) + " ms)");
        const auto in = plucks (4.0);
        const auto og = render (0, settings (0.5f, 0.7f, 0.3f), in);
        auto bloom = [] (const std::vector<float>& x)
        {
            const auto h = band (x, 2500.0, 12000.0);
            double early = 0.0, late = 0.0;
            for (double t0 = 0.6; t0 + 0.6 <= 3.6; t0 += 0.6)
            {
                early += power (h, (size_t) (t0 * sr), (size_t) ((t0 + 0.06) * sr));
                late += power (h, (size_t) ((t0 + 0.08) * sr), (size_t) ((t0 + 0.3) * sr));
            }
            return db (late / early);
        };
        require (bloom (og.l) >= bloom (in.l) + 4.0, "Glass: bright bands bloom after the attack (" + std::to_string (bloom (og.l)) + " vs "
                 + std::to_string (bloom (in.l)) + " dB)");
    }
    {
        // Massive: the body lasts longer; saturation adds density (harmonics), never a sub-octave.
        const auto in = plucks (4.0);
        const auto fm = features (render (1, settings (0.7f, 0.2f, 0.6f), in), 0.6), fi = features (in, 0.6);
        require (fm[9] >= fi[9] + 4.0, "Massive: longer body after the attack (sustain " + std::to_string (fm[9]) + " vs " + std::to_string (fi[9]) + " dB)");
        const auto o = render (1, settings (0.8f, 0.2f, 0.3f), sine (150.0, 0.4, 3.0));
        const double f0 = tone (o.l, 150.0, (size_t) sr, (size_t) (3.0 * sr));
        const double h3 = tone (o.l, 450.0, (size_t) sr, (size_t) (3.0 * sr)), sub = tone (o.l, 75.0, (size_t) sr, (size_t) (3.0 * sr));
        require (db (h3 / f0) > -35.0 && db (sub / f0) < -45.0, "Massive: dense harmonics (3rd " + std::to_string (db (h3 / f0)) + " dB), no sub-octave ("
                 + std::to_string (db (sub / f0)) + " dB)");
    }
    {
        // The repetition measure itself: a 280 ms loop (as the old Hold) is caught, steady noise is not.
        std::vector<float> loop ((size_t) (4.0 * sr)), steady ((size_t) (4.0 * sr));
        uint32_t seed = 99u;
        std::vector<float> grain ((size_t) (0.28 * sr));
        for (size_t n = 0; n < grain.size(); ++n) { seed = seed * 1664525u + 1013904223u; grain[n] = (float) ((double) (seed >> 8) / 8388608.0 - 1.0) * (float) std::exp (-(double) n / (0.05 * sr)); }
        for (size_t n = 0; n < loop.size(); ++n) { loop[n] = grain[n % grain.size()]; seed = seed * 1664525u + 1013904223u; steady[n] = 0.3f * (float) ((double) (seed >> 8) / 8388608.0 - 1.0); }
        require (repetition (loop, 0) > 0.6 && repetition (steady, 0) < 0.6, "the loop detector catches a 280 ms loop (" + std::to_string (repetition (loop, 0)) + ") and passes steady sound ("
                 + std::to_string (repetition (steady, 0)) + ")");
    }
    // 5. Hold keeps a living texture: sustained, no short loop, bounded, released into silence.
    for (int c = 0; c < 4; ++c)
    {
        epinizer::Engine e;
        e.prepare (sr);
        auto s = settings (0.6f, 0.6f, 0.6f);
        s.character = c;
        e.snap (s);
        Stereo chord { std::vector<float> ((size_t) (2.0 * sr)), {} };
        for (size_t n = 0; n < chord.l.size(); ++n)
            for (double hz : { 220.0, 277.18, 329.63 }) chord.l[n] += (float) (0.12 * std::sin (twoPi * hz * (double) n / sr));
        chord.r = chord.l;
        e.process (chord.l.data(), chord.r.data(), (int) chord.l.size(), s);
        s.hold = true;
        Stereo held { std::vector<float> ((size_t) (6.0 * sr)), std::vector<float> ((size_t) (6.0 * sr)) };
        e.process (held.l.data(), held.r.data(), (int) held.l.size(), s);
        const double first = power (held.l, (size_t) sr, (size_t) (2.0 * sr)), last = power (held.l, (size_t) (5.0 * sr), (size_t) (6.0 * sr));
        require (first > 1.0e-6 && last > first * 0.25 && last < first * 4.0, std::string (names[c]) + ": Hold sustains (" + std::to_string (db (last / first)) + " dB over 4 s)");
        const double worst = repetition (held.l, (size_t) (2.0 * sr));
        require (worst < 0.6, std::string (names[c]) + ": Hold has no short loop (repetition " + std::to_string (worst) + ")");
        float peak = 0.0f;
        for (float v : held.l) peak = std::max (peak, std::abs (v));
        require (peak < 1.0f, std::string (names[c]) + ": Hold bounded");
        s.hold = false;
        Stereo rel { std::vector<float> ((size_t) (14.0 * sr)), std::vector<float> ((size_t) (14.0 * sr)) };
        e.process (rel.l.data(), rel.r.data(), (int) rel.l.size(), s);
        require (power (rel.l, (size_t) (13.0 * sr), rel.l.size()) < 1.0e-8, std::string (names[c]) + ": released Hold fades out");
        e.reset();
        s.hold = true;
        Stereo empty { std::vector<float> ((size_t) sr), std::vector<float> ((size_t) sr) };
        e.process (empty.l.data(), empty.r.data(), (int) empty.l.size(), s);
        require (power (empty.l, 0, empty.l.size()) == 0.0, std::string (names[c]) + ": reset clears what Hold captured");
    }
    // 6. Comparable level: wet as loud as dry (A/B at the same loudness), silence stays silence.
    for (int c = 0; c < 4; ++c)
        for (int src = 0; src < 2; ++src)
        {
            const auto in = src == 0 ? plucks (6.0) : drums (6.0);
            const auto out = render (c, settings (0.6f, 0.5f, 0.5f, true), in);
            const double d = db (power (out.l, (size_t) (2.0 * sr), out.l.size()) / power (in.l, (size_t) (2.0 * sr), in.l.size()));
            require (std::abs (d) <= 3.0, std::string (names[c]) + (src ? " on drums" : " on plucks") + ": wet as loud as dry (" + std::to_string (d) + " dB)");
        }
    for (int c = 0; c < 4; ++c)
    {
        Stereo silent { std::vector<float> ((size_t) sr), std::vector<float> ((size_t) sr) };
        const auto out = render (c, settings (1.0f, 1.0f, 1.0f, true), silent);
        require (power (out.l, 0, out.l.size()) == 0.0 && power (out.r, 0, out.r.size()) == 0.0, std::string (names[c]) + ": silence stays silence");
    }
    std::cout << (failures ? "FAILED " : "PASS ") << (checks - failures) << "/" << checks << " character checks\n";
    return failures ? 1 : 0;
}
