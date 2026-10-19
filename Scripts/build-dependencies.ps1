<#
.LICENSE
Copyright (c) 2026 DLR e.V.

This program and the accompanying materials are made
available under the terms of the Eclipse Public License 2.0
which is available at https://www.eclipse.org/legal/epl-2.0/

SPDX-License-Identifier: EPL-2.0

.SYNOPSIS
Builds the esmini dependencies for ue_esmini_feature natively on Windows.

.DESCRIPTION
Clones the esmini version used by this plugin, configures a minimal Win64
Release build with CMake and Visual Studio, and copies the headers, DLLs and
import library expected by ue_esmini_featureRuntime.Build.cs into ThirdParty.

.REQUIREMENTS
- Windows 10/11 x64
- Git in PATH
- CMake 3.21 or newer in PATH
- Visual Studio 2022 or Build Tools 2022 with "Desktop development with C++"
  and a Windows SDK installed
- Internet access for the esmini clone and its CMake dependencies

.EXAMPLE
PS> .\Scripts\build_dependencies.ps1

.EXAMPLE
PS> .\Scripts\build_dependencies.ps1 -Clean

.EXAMPLE
PS> .\Scripts\build_dependencies.ps1 -BuildRoot C:\Build\ue-esmini
#>

[CmdletBinding()]
param(
    [string]$EsminiVersion = "v3.5.0",
    [string]$BuildRoot = "",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Toolset = "",
    [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true, Position = 0)]
        [string]$Command,

        [Parameter(Position = 1)]
        [string[]]$Arguments = @()
    )

    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Command $($Arguments -join ' ')"
    }
}

function Assert-CommandAvailable {
    param([Parameter(Mandatory = $true)][string]$Name)

    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' was not found in PATH. See Get-Help $PSCommandPath -Full."
    }
}

function Find-BuildArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string[]]$SearchRoots
    )

    $matches = @(
        foreach ($root in $SearchRoots) {
            if (Test-Path -LiteralPath $root) {
                Get-ChildItem -LiteralPath $root -Filter $Name -File -Recurse |
                    Where-Object { $_.FullName -match '[\\/]Release[\\/]' -or $_.DirectoryName -eq $root }
            }
        }
    )

    $artifact = $matches | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if (-not $artifact) {
        throw "Build succeeded, but '$Name' could not be found below: $($SearchRoots -join ', ')"
    }

    return $artifact.FullName
}

if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT) {
    throw "This script builds native Windows dependencies and must be run on Windows."
}

Assert-CommandAvailable git
Assert-CommandAvailable cmake

$cmakeVersion = (cmake --version | Select-Object -First 1)
if ($cmakeVersion -notmatch 'cmake version (\d+)\.(\d+)') {
    throw "Could not determine the installed CMake version."
}
if ([int]$Matches[1] -lt 3 -or ([int]$Matches[1] -eq 3 -and [int]$Matches[2] -lt 21)) {
    throw "CMake 3.21 or newer is required for the Visual Studio 2022 generator. Found: $cmakeVersion"
}

$scriptDirectory = Split-Path -Parent $PSCommandPath
$pluginRoot = Split-Path -Parent $scriptDirectory
if (-not $BuildRoot) {
    $BuildRoot = Join-Path $pluginRoot ".build"
}
$thirdPartyDirectory = Join-Path $pluginRoot "Source\ue_esmini_featureRuntime\ThirdParty\esmini"
$includeDirectory = Join-Path $thirdPartyDirectory "Include"
$libraryDirectory = Join-Path $thirdPartyDirectory "Lib"
$sourceDirectory = Join-Path $BuildRoot "esmini-$($EsminiVersion.TrimStart('v'))"
$buildDirectory = Join-Path $sourceDirectory "build-ue-win64"

if ($Clean -and (Test-Path -LiteralPath $sourceDirectory)) {
    Write-Host "Removing previous source and build directory: $sourceDirectory"
    Remove-Item -LiteralPath $sourceDirectory -Recurse -Force
}

New-Item -ItemType Directory -Path $BuildRoot -Force | Out-Null

if (-not (Test-Path -LiteralPath (Join-Path $sourceDirectory ".git"))) {
    if (Test-Path -LiteralPath $sourceDirectory) {
        throw "'$sourceDirectory' exists but is not an esmini Git checkout. Use -Clean or another -BuildRoot."
    }

    Write-Host "Cloning esmini $EsminiVersion..."
    Invoke-NativeCommand git @(
	    "clone",
        "--depth",
		"1",
		"--branch",
		"$EsminiVersion",
		"https://github.com/esmini/esmini.git",
		$sourceDirectory
	)
}
else {
    Write-Host "Refreshing existing esmini checkout..."
    Invoke-NativeCommand git @(
        "-C",
        $sourceDirectory,
        "fetch",
        "--depth",
        "1",
        "origin",
        "refs/tags/${EsminiVersion}:refs/tags/${EsminiVersion}"
    )
	
    Invoke-NativeCommand git @(
        "-C",
        $sourceDirectory,
        "checkout",
        "--force",
        $EsminiVersion
    )

    Invoke-NativeCommand git @(
        "-C",
        $sourceDirectory,
        "reset",
        "--hard",
        $EsminiVersion
    )
}

$configureArguments = @(
    "-S", $sourceDirectory,
    "-B", $buildDirectory,
    "-G", $Generator,
    "-A", "x64",
    "-DBUILD_EXAMPLES=OFF",
    "-DBUILD_ODRPLOT=OFF",
    "-DBUILD_REPLAYER=OFF",
    "-DBUILD_TESTING=OFF",
    "-DUSE_GTEST=OFF",
    "-DUSE_IMPLOT=OFF",
    "-DUSE_OSG=OFF",
    "-DUSE_OSI=OFF",
    "-DUSE_SUMO=OFF"
)
if ($Toolset) {
    $configureArguments += @("-T", $Toolset)
}

Write-Host "Configuring native Win64 Release build..."
Invoke-NativeCommand cmake $configureArguments

Write-Host "Building esminiLib and esminiRMLib..."
Invoke-NativeCommand cmake @(
	"--build",
	$buildDirectory,
	"--config",
	"Release",
	"--target",
	"esminiLib",
	"esminiRMLib",
	"--parallel"
	)

$artifactSearchRoots = @($buildDirectory, (Join-Path $sourceDirectory "bin"))
$artifacts = @{
    "esminiLib.dll"   = Find-BuildArtifact "esminiLib.dll" $artifactSearchRoots
    "esminiLib.lib"   = Find-BuildArtifact "esminiLib.lib" $artifactSearchRoots
    "esminiRMLib.dll" = Find-BuildArtifact "esminiRMLib.dll" $artifactSearchRoots
}

New-Item -ItemType Directory -Path $includeDirectory, $libraryDirectory -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $sourceDirectory "EnvironmentSimulator\Libraries\esminiLib\esminiLib.hpp") `
    -Destination $includeDirectory -Force
Copy-Item -LiteralPath (Join-Path $sourceDirectory "EnvironmentSimulator\Libraries\esminiRMLib\esminiRMLib.hpp") `
    -Destination $includeDirectory -Force

foreach ($artifactName in $artifacts.Keys) {
    Copy-Item -LiteralPath $artifacts[$artifactName] `
        -Destination (Join-Path $libraryDirectory $artifactName) -Force
}

Write-Host ""
Write-Host "Built esmini $EsminiVersion for Win64 and refreshed:"
Write-Host "  $includeDirectory"
Write-Host "  $libraryDirectory"
Write-Host "Unreal Build Tool will stage the two DLLs into the plugin's Binaries\Win64 directory."
