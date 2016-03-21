:Android Build script for Windows platform
: Author Keith Yan

for %%c in (g711)do (
	echo building %%c begin
	copy Android.%%c.mk Android.mk
	call :writeAppMK %%c
	call ndk-build
	call :determinResult %%c
)

PAUSE
goto :eof

:writeAppMK
echo APP_MODULES := lib%1 > Application.mk
echo APP_OPTIM := debug >> Application.mk
echo APP_ABI := armeabi-v7a >> Application.mk
echo APP_STL := c++_shared >> Application.mk
echo APP_PLATFORM := android-16 >> Application.mk
goto :eof

:determinResult
if %errorlevel% NEQ 0 (
   echo build %1 failed
)
goto :eof