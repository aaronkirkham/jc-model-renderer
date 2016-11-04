QT              +=  core widgets gui opengl
LIBS            +=  opengl32.lib glu32.lib

INCLUDEPATH     +=  $$PWD/include

TARGET          =   jc3-rbm-renderer
TEMPLATE        =   app

SOURCES         +=  src/main.cpp\
                    src/engine/RBMLoader.cpp \
                    src/engine/RenderBlockFactory.cpp \
                    src/MainWindow.cpp \
                    src/Renderer.cpp \
                    src/FileReader.cpp \
                    src/Buffer.cpp \
                    src/engine/Materials.cpp

HEADERS         +=  include/engine/renderblocks/RenderBlockGeneralJC3.h \
                    include/engine/renderblocks/RenderBlockCharacter.h \
                    include/engine/renderblocks/RenderBlockCarPaintMM.h \
                    include/engine/RBMLoader.h \
                    include/engine/RenderBlockFactory.h \
                    include/MainWindow.h \
                    include/Renderer.h \
                    include/engine/renderblocks/IRenderBlock.h \
                    include/Buffer.h \
                    include/Singleton.h \
                    include/FileReader.h \
                    include/engine/Materials.h \
                    include/engine/renderblocks/RenderBlockCharacter.h \
                    include/engine/renderblocks/RenderBlockBuildingJC3.h \
                    include/engine/renderblocks/RenderBlockLandmark.h \
                    include/engine/VertexTypes.h

FORMS           +=  resources/mainwindow.ui
RESOURCES       +=  resources/resources.qrc

debug:DESTDIR   =   build/debug
release:DESTDIR =   build/release

OBJECTS_DIR     =   $$DESTDIR/output/.obj
MOC_DIR         =   $$DESTDIR/output/.moc
RCC_DIR         =   $$DESTDIR/output/.rcc
UI_DIR          =   $$DESTDIR/output/.ui
