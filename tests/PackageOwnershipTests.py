"""macOS packaging regression: compare with pkgbuild's root-owned reference."""
import gzip
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "ownership", Path(__file__).resolve().parents[1] / "scripts/root-pkg-ownership.py")
ownership = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ownership)


class PackageOwnershipTests(unittest.TestCase):
    def test_matches_native_recommended_ownership(self):
        with tempfile.TemporaryDirectory(prefix="epinizer-pkg-test-", dir="/private/tmp") as temp:
            base = Path(temp)
            root = base / "root"
            root.mkdir()
            (root / "file with spaces").write_bytes(b"\x00plugin data\xff")
            (root / "executable").write_text("#!/bin/sh\nexit 0\n")
            (root / "executable").chmod(0o755)
            (root / "Contents").mkdir()
            (root / "Contents" / "metadata").write_text("bundle metadata")
            for mode in ("recommended", "preserve"):
                package = base / (mode + ".pkg")
                subprocess.run(["pkgbuild", "--root", str(root), "--ownership", mode,
                                "--identifier", "audio.epinizer.ownership-test", "--version", "1",
                                "--install-location", "/", str(package)], check=True,
                               capture_output=True, env=dict(os.environ, COPYFILE_DISABLE="1"))
                subprocess.run(["pkgutil", "--expand", str(package), str(base / mode)],
                               check=True, capture_output=True)
            ownership.normalize(base / "preserve")
            payloads = []
            boms = []
            for mode in ("recommended", "preserve"):
                with gzip.open(base / mode / "Payload", "rb") as stream:
                    payloads.append(stream.read())
                boms.append(subprocess.check_output(["lsbom", str(base / mode / "Bom")]))
            self.assertEqual(payloads[0], payloads[1], "Payload must match Apple's reference")
            def records(bom):
                result = []
                for line in bom.decode().splitlines():
                    fields = line.split("\t")
                    # Directory records have no file data; trailing zeros are unused.
                    result.append(fields[:3] if int(fields[1], 8) & 0o170000 == 0o040000 else fields)
                return result
            self.assertEqual(records(boms[0]), records(boms[1]),
                             "BOM paths/modes/checksums/owners must match")

    def test_rejects_invalid_or_truncated_payload(self):
        for malformed in (b"", b"bad archive", b"070707" + b"0" * 70):
            with self.subTest(payload=malformed), self.assertRaises(ValueError):
                ownership.root_payload(malformed)


if __name__ == "__main__":
    unittest.main()
