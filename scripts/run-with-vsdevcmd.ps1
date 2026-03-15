$ErrorActionPreference = 'Stop'

$commandArgs = @($args)
if ($commandArgs.Count -gt 0 -and $commandArgs[0] -eq '--') {
    if ($commandArgs.Count -eq 1) {
        $commandArgs = @()
    }
    else {
        $commandArgs = $commandArgs[1..($commandArgs.Count - 1)]
    }
}

if (-not $commandArgs -or $commandArgs.Count -eq 0) {
    throw 'No command provided. Usage: run-with-vsdevcmd.ps1 -- <command> [args...]'
}

$programFilesX86 = [Environment]::GetEnvironmentVariable('ProgramFiles(x86)')
$programFiles = [Environment]::GetEnvironmentVariable('ProgramFiles')

$vswhereCandidates = @(@(
        (Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'),
        (Join-Path $programFiles 'Microsoft Visual Studio\Installer\vswhere.exe')
    ) | Where-Object { $_ -and (Test-Path $_) })

if (-not $vswhereCandidates -or $vswhereCandidates.Count -eq 0) {
    throw 'vswhere.exe not found. Install Visual Studio Build Tools with C++ components.'
}

$vswhere = $vswhereCandidates[0]
$vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

if (-not $vsInstall) {
    throw 'No Visual Studio installation with C++ tools was found.'
}

$devCmd = Join-Path $vsInstall 'Common7\Tools\VsDevCmd.bat'
if (-not (Test-Path $devCmd)) {
    throw "VsDevCmd.bat not found at: $devCmd"
}

$innerCommand = $commandArgs -join ' '
$cmdLine = ('"{0}" -arch=x64 -host_arch=x64 && {1}' -f $devCmd, $innerCommand)

& cmd.exe /d /s /c $cmdLine
exit $LASTEXITCODE
