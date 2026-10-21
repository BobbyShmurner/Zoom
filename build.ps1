param (
	[string] $platform = "windows",
	[switch] $NoRun
)

$buildArgs = @("build", "-p", $platform)
if ($platform -eq "windows" -and $env:CMAKE_GENERATOR -ceq "Ninja") {
	# Geode otherwise adds -A x64, which Ninja does not support.
	$buildArgs += "--ninja"
}
geode @buildArgs

if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if ($NoRun) { return }

if ($platform -eq "windows") {
	geode run
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
