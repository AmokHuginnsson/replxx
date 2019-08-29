#! /bin/sh

while [ ${#} -gt 0 ] ; do
	if [ \( ${#} -gt 0 \) -a \( "x${1}" = "xpurge" \) ] ; then
		/bin/rm -rf build
		shift
		continue
	fi

	if [ \( ${#} -gt 0 \) -a \( "x${1}" = "xsudo" \) ] ; then
		sudo="sudo"
		shift
		continue
	fi

	if [ \( ${#} -gt 0 \) -a \( "x${1}" = "xmsvcxx" \) ] ; then
		msvcxx=1
		shift
		continue
	fi

	if [ ${#} -gt 0 ] ; then
		installPrefix="-DCMAKE_INSTALL_PREFIX=${1}"
		shift
		continue
	fi
done

build_target() {
	target="${1}"
	mkdir -p "build/${target}"
	cd "build/${target}"
	if [ "x${msvcxx}" = "x1" ] ; then
		CMAKE="/cygdrive/c/Program Files/CMake/bin/cmake.exe"
		"${CMAKE}" -G 'Visual Studio 14' ${installPrefix} ../../
	  "${CMAKE}" --build . --config Debug
		"${CMAKE}" --build . --config Release
		"${CMAKE}" --build . --config Debug --target Install
		"${CMAKE}" --build . --config Release --target Install
	else
		cmake -DCMAKE_BUILD_TYPE=${target} ${installPrefix} ../../
		make
		${sudo} make install
	fi
	cd ../..
}

for target in debug release ; do
	build_target "${target}"
done

