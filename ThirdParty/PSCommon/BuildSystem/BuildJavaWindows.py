import os
import subprocess
import sys
import shutil

# parse command line
if len(sys.argv) < 5:
    print("usage: " + sys.argv[0] + " <x86|x64> <BinDir> <SourceDir> <Name> [NeededJarFiles] [MainClass]")
    exit(1)

platform_string = ""
if sys.argv[1] == "" or sys.argv[1] == "x86":
    platform_string = "Win32"
elif sys.argv[1] == "x64":
    platform_string = "x64"
else:
    print('First argument must be "x86", "x64" or empty (x86)')
    exit(1)
    
bin_dir = os.path.abspath(sys.argv[2])
source_dir = os.path.abspath(sys.argv[3])
proj_name = sys.argv[4]
int_dir = os.path.join(bin_dir, "Intermediate", platform_string + "-Release", proj_name)
needed_jar_files = ""
main_class = ""
if len(sys.argv) > 5:
    needed_jar_files = sys.argv[5]
if len(sys.argv) > 6:
    main_class = sys.argv[6]

RELEASE_DIR = os.path.abspath(os.path.join(bin_dir, platform_string + "-Release"))
DEBUG_DIR = os.path.abspath(os.path.join(bin_dir, platform_string + "-Debug"))

JAR_FILE = os.path.join(RELEASE_DIR, proj_name + '.jar')
BATCH_FILE = os.path.join(RELEASE_DIR, proj_name + '.bat')

# make sure JAVA_HOME is set
JAVA_HOME = os.path.expandvars("$JAVA_HOME")
if JAVA_HOME == "":
    print("JAVA_HOME is not set!")
    exit(1)
    
CLASS_PATH = os.path.expandvars("$CLASSPATH")

TEMP_BUILD_DIR = int_dir

# create bin dir if needed
if not os.path.exists(RELEASE_DIR):
    os.makedirs(RELEASE_DIR)
if not os.path.exists(DEBUG_DIR):
    os.makedirs(DEBUG_DIR)
if not os.path.exists(TEMP_BUILD_DIR):
    os.makedirs(TEMP_BUILD_DIR)
 
# build
cmd = [os.path.join(JAVA_HOME, 'bin\javac.exe')]
if needed_jar_files != "":
    # add class path
    cp = ''
    needed_list = needed_jar_files.split(';')
    for needed in needed_list:
        cp += os.path.join(RELEASE_DIR, needed) + ';'
    cp += CLASS_PATH + ';'
    cmd.append('-cp')
    cmd.append(cp)

cmd.append('-d')
cmd.append(TEMP_BUILD_DIR)
cmd.append('-Xlint:unchecked')

cmd.append(os.path.join(source_dir, '*.java'))
subprocess.check_call(cmd)

# create JAR file
cmd = [os.path.join(JAVA_HOME, 'bin\jar.exe')]
need_manifest = main_class != "" or needed_jar_files != ""
if need_manifest:
    cmd.append('-cfm')
    # add manifest
    TEMP_MANIFEST_FILE = os.path.join(int_dir, "Manifest.txt")
    manifest_file = open(TEMP_MANIFEST_FILE, 'w')
    if needed_jar_files != "":
        manifest_file.write("Class-Path:")
        needed_list = needed_jar_files.split(';')
        for needed in needed_list:
            manifest_file.write(" " + needed)
        manifest_file.write('\n')
    if main_class != "":
        manifest_file.write("Main-Class: " + main_class + "\n")
    manifest_file.close()
else:
    cmd.append('-cf')

cmd.append(JAR_FILE)

if need_manifest:
    cmd.append(TEMP_MANIFEST_FILE)
    
cmd.append('-C')
cmd.append(TEMP_BUILD_DIR)
cmd.append('.')
subprocess.check_call(cmd)

# copy jar to Bin/Debug
shutil.copy(JAR_FILE, DEBUG_DIR)
    
# create batch file (by default, windows does not open a console when double-clicking jar files)
if main_class != "":
    print("Creating batch file...")
    batch = open(BATCH_FILE, 'w')
    batch.write('java -Xmx768m -jar ' + proj_name + '.jar\n')
    batch.close()

    # copy batch to Bin/Debug
    shutil.copy(BATCH_FILE, DEBUG_DIR)
