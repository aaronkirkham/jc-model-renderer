QT              +=  core widgets gui opengl
LIBS            +=  opengl32.lib glu32.lib

INCLUDEPATH     +=  $$PWD/include

TARGET          =   jc3-rbm-renderer
TEMPLATE        =   app

SOURCES         +=  src/main.cpp\
                    src/engine/RBMLoader.cpp \
                    src/engine/RenderBlockFactory.cpp \
                    src/MainWindow.cpp \
                    src/Renderer.cpp

HEADERS         +=  include/engine/renderblocks/RenderBlockGeneralJC3.h \
                    include/engine/RBMLoader.h \
                    include/engine/RenderBlockFactory.h \
                    include/MainWindow.h \
                    include/Renderer.h \
                    include/engine/renderblocks/IRenderBlock.h \
                    include/engine/Buffer.h

FORMS           +=  resources/mainwindow.ui

DISTFILES       +=  resources/shaders/vertexshader.glsl \
                    resources/shaders/fragmentshader.glsl

debug:DESTDIR   =   build/debug
release:DESTDIR =   build/release

OBJECTS_DIR     =   $$DESTDIR/output/.obj
MOC_DIR         =   $$DESTDIR/output/.moc
RCC_DIR         =   $$DESTDIR/output/.rcc
UI_DIR          =   $$DESTDIR/output/.ui
