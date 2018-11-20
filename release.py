import os
from subprocess import call
from zipfile import ZipFile

def GetCurrentVersion():
  version = []
  with open("src/version.h") as f:
    for line in f.readlines():
      if not "#define VERSION_" in line:
        continue

      version.append(str([int(s) for s in line.split() if s.isdigit()][0]))

  return ".".join(version)

VERSION = GetCurrentVersion()

# create the application and assets zip
zipf = ZipFile("jc3-rbm-renderer.zip", "w")
zipf.write("out/Release/jc3-rbm-renderer.exe", "jc3-rbm-renderer.exe")
zipf.write("assets/shaders/carlight.fb")
zipf.write("assets/shaders/carpaintmm.fb")
zipf.write("assets/shaders/carpaintmm.vb")
zipf.write("assets/shaders/carpaintmm_deform.vb")
zipf.write("assets/shaders/carpaintmm_skinned.vb")
zipf.write("assets/shaders/character.fb")
zipf.write("assets/shaders/character.vb")
zipf.write("assets/shaders/character2uvs.vb")
zipf.write("assets/shaders/character3uvs.vb")
zipf.write("assets/shaders/character8.vb")
zipf.write("assets/shaders/character82uvs.vb")
zipf.write("assets/shaders/character83uvs.vb")
zipf.write("assets/shaders/characterhair_msk.fb")
zipf.write("assets/shaders/characterskin.fb")
zipf.write("assets/shaders/characterskin.vb")
zipf.write("assets/shaders/characterskin2uvs.vb")
zipf.write("assets/shaders/characterskin3uvs.vb")
zipf.write("assets/shaders/characterskin8.vb")
zipf.write("assets/shaders/characterskin82uvs.vb")
zipf.write("assets/shaders/characterskin83uvs.vb")
zipf.write("assets/shaders/generaljc3.fb")
zipf.write("assets/shaders/generaljc3.vb")
zipf.write("assets/shaders/generalmkiii.fb")
zipf.write("assets/shaders/generalmkiii.vb")
zipf.write("assets/shaders/landmark.fb")
zipf.write("assets/shaders/landmark.vb")
zipf.write("assets/shaders/window.fb")
zipf.write("assets/shaders/window.vb")
zipf.close()

# create the artifacts zip
zipf = ZipFile("artifacts.zip", "w")
zipf.write("out/Release/jc3-rbm-renderer.pdb", "jc3-rbm-renderer.pdb")
zipf.write("out/Release/imgui/imgui.pdb", "imgui.pdb")
zipf.write("out/Release/jc3-rbm-renderer/vc141.pdb", "vc141.pdb")
zipf.write("out/Release/zlib/zlib.pdb", "zlib.pdb")
zipf.close()

# create the output directory
os.makedirs("out/bin")

# copy files to output directory
os.rename("jc3-rbm-renderer.zip", "out/bin/jc3-rbm-renderer-{0}.zip".format(VERSION))
os.rename("artifacts.zip", "out/bin/artifacts.zip")