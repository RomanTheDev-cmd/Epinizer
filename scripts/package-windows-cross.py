"""Package cross-built Windows binaries without claiming a Windows runtime test."""
from pathlib import Path
import shutil
import json
import re
import tempfile
import zipfile
root = Path(__file__).resolve().parent.parent
build = root / 'build-windows-v2-msvc'
out = root / 'dist' / 'Epinizer-2.0.0-Windows-x64.zip'
plugin = build / 'Epinizer_artefacts/Release/VST3/Epinizer.vst3'
if not plugin.is_dir():
    raise SystemExit('Build the Windows VST3 first')
manifest = plugin / 'Contents/Resources/moduleinfo.json'
if manifest.exists():
    # JUCE's helper permits JSON trailing commas; emit strict JSON for other readers.
    metadata = json.loads(re.sub(r',(\s*[}\]])', r'\1', manifest.read_text()))
    manifest.write_text(json.dumps(metadata, indent=2) + '\n')
with tempfile.TemporaryDirectory(prefix='epinizer-windows-package-') as tmp:
    stage = Path(tmp) / 'Epinizer-Windows'
    stage.mkdir()
    shutil.copytree(plugin, stage / plugin.name)
    for name in ['install-windows.ps1', 'Install-Epinizer.cmd', 'Validate-Epinizer.cmd', 'validate-windows.ps1']:
        shutil.copy2(root / 'scripts' / name, stage / name)
    for name in ['README.md', 'THIRD_PARTY.md', 'VALIDATION.md', 'CHANGELOG.md']:
        shutil.copy2(root / name, stage / name)
    checks = stage / 'validation'
    checks.mkdir()
    shutil.copy2(build / 'epinizer_dsp_tests.exe', checks)
    shutil.copy2(build / 'epinizer_character_tests.exe', checks)
    shutil.copy2(build / 'epinizer_realtime_tests.exe', checks)
    for name in ['epinizer_plugin_tests', 'epinizer_vst3_host_tests']:
        shutil.copy2(build / f'{name}_artefacts/Release/{name}.exe', checks)
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for file in sorted(stage.rglob('*')):
            if file.is_file():
                archive.write(file, file.relative_to(Path(tmp)))
print(out)
