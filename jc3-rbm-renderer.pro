QT              +=      core widgets gui opengl

LIBS            +=      opengl32.lib glu32.lib

TARGET          = jc3-rbm-renderer
TEMPLATE        = app

SOURCES         +=  main.cpp\
                    engine/RBMLoader.cpp \
                    engine/RenderBlockFactory.cpp \
                    MainWindow.cpp \
                    Renderer.cpp

HEADERS         +=  engine/renderblocks/RenderBlockGeneralJC3.h \
                    engine/RBMLoader.h \
                    engine/RenderBlockFactory.h \
                    MainWindow.h \
                    Renderer.h \
                    engine/renderblocks/IRenderBlock.h \
                    engine/Buffer.h

FORMS           +=  mainwindow.ui

DISTFILES       +=  shaders/vertexshader.glsl \
                    shaders/fragmentshader.glsl

debug:DESTDIR   = build/debug
release:DESTDIR = build/release

OBJECTS_DIR     = $$DESTDIR/output/.obj
MOC_DIR         = $$DESTDIR/output/.moc
RCC_DIR         = $$DESTDIR/output/.rcc
UI_DIR          = $$DESTDIR/output/.ui
