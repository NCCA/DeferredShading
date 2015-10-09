TARGET=DeferredShading
OBJECTS_DIR=obj
# as I want to support 4.8 and 5 this will set a flag for some of the mac stuff
# mainly in the types.h file for the setMacVisual which is native in Qt5
isEqual(QT_MAJOR_VERSION, 5) {
	cache()
	DEFINES +=QT5BUILD
}



MOC_DIR=moc
CONFIG-=app_bundle
QT+=gui opengl core
SOURCES+= src/main.cpp \
        src/NGLScene.cpp \
        src/ScreenQuad.cpp \
        src/PointLight.cpp

HEADERS+= include/NGLScene.h \
        include/ScreenQuad.h \
        include/PointLight.h
INCLUDEPATH +=./include

DESTDIR=./
OTHER_FILES+= shaders/Pass1Vert.glsl \
              shaders/Pass1Frag.glsl \
              shaders/Pass2Vert.glsl \
              shaders/Pass2Frag.glsl \
              shaders/DebugVert.glsl \
              shaders/DebugFrag.glsl \
              shaders/LightingPassVert.glsl \
              shaders/LightingPassFrag.glsl
CONFIG += console
CONFIG -= app_bundle

# note each command you add needs a ; as it will be run as a single line
# first check if we are shadow building or not easiest way is to check out against current
!equals(PWD, $${OUT_PWD}){
	copydata.commands = echo "creating destination dirs" ;
	# now make a dir
	copydata.commands += mkdir -p $$OUT_PWD/shaders ;
	copydata.commands += echo "copying files" ;
	# then copy the files
	copydata.commands += $(COPY_DIR) $$PWD/shaders/* $$OUT_PWD/shaders/ ;
	# now make sure the first target is built before copy
	first.depends = $(first) copydata
	export(first.depends)
	export(copydata.commands)
	# now add it as an extra target
	QMAKE_EXTRA_TARGETS += first copydata
}


NGLPATH=$$(NGLDIR)
isEmpty(NGLPATH){ # note brace must be here
	message("including $HOME/NGL")
	include($(HOME)/NGL/UseNGL.pri)
}
else{ # note brace must be here
	message("Using custom NGL location")
	include($(NGLDIR)/UseNGL.pri)
}
