param (
	[string] $platform = "windows",
	[switch] $NoRun,
	[string] $Profile
)

if ($Profile -and $platform -ne "windows") {
	throw "Profiles are only supported for Windows builds."
}

if ($Profile) {
	$profileDir = geode profile path --dir $Profile
	if ($LASTEXITCODE -ne 0 -or -not $profileDir) { throw "Could not find profile '$Profile'." }
}

$buildArgs = @("build", "-p", $platform)
if ($platform -eq "windows" -and ($env:CMAKE_GENERATOR -ceq "Ninja" -or (Test-Path "$PSScriptRoot/build/build.ninja"))) {
	# Geode otherwise adds -A x64, which Ninja does not support.
	$buildArgs += "--ninja"
}
if ($platform -eq "windows") {
	# Package installation uses the CLI's default profile, so install explicitly
	# when another profile was requested. Reset the cached option otherwise.
	$dontInstall = if ($Profile) { "ON" } else { "OFF" }
	$buildArgs += "--", "-DGEODE_DONT_INSTALL_MODS=$dontInstall"
}
geode @buildArgs

if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if ($Profile) {
	$modsDir = Join-Path $profileDir "geode/mods"
	if (-not (Test-Path $modsDir)) { throw "Geode mods directory not found: $modsDir" }
	Copy-Item "$PSScriptRoot/build/bobby_shmurner.zoom.geode" $modsDir -Force -ErrorAction Stop
}
if ($NoRun) { return }

if ($platform -eq "windows") {
	if ($Profile) { geode profile run $Profile } else { geode run }
} elseif ($platform -eq "mac-os") {
	env SteamAppId=322170 SteamGameId=322170 geode run
} elseif ($platform -eq "android64") {
	adb push $PSScriptRoot/build-android64/bobby_shmurner.zoom.geode /storage/emulated/0/Android/media/com.geode.launcher/game/geode/mods/bobby_shmurner.zoom.geode
	adb shell am force-stop com.geode.launcher
	adb shell am start -D -n "com.geode.launcher/com.geode.launcher.MainActivity"

	$gdpid = $null
	while ([string]::IsNullOrEmpty($gdpid)) {
		Start-Sleep -Milliseconds 100
		$gdpid = adb shell pidof com.geode.launcher
	}

	try {
		adb logcat --pid $gdpid
	}
	finally {
		adb shell am force-stop com.geode.launcher
	}
} else {
	Write-Warning "Cannot auto-run $platform"
}
