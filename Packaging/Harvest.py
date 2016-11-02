#!/usr/bin/python2.7

#/****************************************************************************
#*                                                                           *
#*  OpenNI 2.x Alpha                                                         *
#*  Copyright (C) 2012 PrimeSense Ltd.                                       *
#*                                                                           *
#*  This file is part of OpenNI.                                             *
#*                                                                           *
#*  Licensed under the Apache License, Version 2.0 (the "License");          *
#*  you may not use this file except in compliance with the License.         *
#*  You may obtain a copy of the License at                                  *
#*                                                                           *
#*      http://www.apache.org/licenses/LICENSE-2.0                           *
#*                                                                           *
#*  Unless required by applicable law or agreed to in writing, software      *
#*  distributed under the License is distributed on an "AS IS" BASIS,        *
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
#*  See the License for the specific language governing permissions and      *
#*  limitations under the License.                                           *
#*                                                                           *
#****************************************************************************/
import os
import glob
import re
import sys
import shutil
import platform
import stat
import xml.dom.minidom

class Harvest:
    def __init__(self, rootDir, outDir, arch, osName):
        self.rootDir = rootDir
        self.outDir = outDir
        self.arch = arch
        self.osName = osName
        self.platformSuffix = ''
        self.glutSuffix = '32'
        self.binDir = os.path.join(rootDir, 'Bin', arch + '-Release')

        # override some defaults
        if self.osName == 'Windows':
            if arch == 'x86':
                self.binDir = os.path.join(rootDir, 'Bin', 'Win32-Release')
            elif arch == 'x64':
                self.platformSuffix = '64'
                self.glutSuffix = '64'
        elif self.osName == 'Android':
            self.binDir = os.path.join(rootDir, 'Wrappers', 'java', 'libs', 'armeabi-v7a')

    def copySharedObject(self, sourceDir, name, targetDir):
        if self.osName == 'Windows':
            shutil.copy(os.path.join(sourceDir, name + '.dll'), targetDir)
            shutil.copy(os.path.join(sourceDir, name + '.pdb'), targetDir)
        elif self.osName == 'Linux' or self.osName == 'Android':
            shutil.copy(os.path.join(sourceDir, 'lib' + name + '.so'), targetDir)
        elif self.osName == 'Darwin':
            shutil.copy(os.path.join(sourceDir, 'lib' + name + '.dylib'), targetDir)
        else:
            raise 'Unsupported platform!'

    def copyExecutable(self, sourceDir, name, targetDir):
        if self.osName == 'Windows':
            shutil.copy(os.path.join(sourceDir, name + '.exe'), targetDir)
            shutil.copy(os.path.join(sourceDir, name + '.pdb'), targetDir)
        else:
            shutil.copy(os.path.join(sourceDir, name), targetDir)

    def regxReplace(self, findStr, repStr, filePath):
        "replaces all findStr by repStr in file filePath using regular expression"
        findStrRegx = re.compile(findStr)
        tempName = filePath+'~~~'
        fileMode = os.stat(filePath).st_mode
        os.chmod(filePath, fileMode | stat.S_IWRITE)
        input = open(filePath)
        output = open(tempName, 'w')
        for s in input:
            output.write(findStrRegx.sub(repStr, s))
        output.close()
        input.close()
        os.remove(filePath)
        os.rename(tempName, filePath)

    def copyRedistFiles(self, targetDir):
        os.makedirs(targetDir)
        # start with OpenNI itself
        self.copySharedObject(self.binDir, 'OpenNI2', targetDir)
        self.copySharedObject(self.binDir, 'OpenNI2.jni', targetDir)

        if self.osName != 'Android':
            shutil.copy(os.path.join(self.binDir, 'org.openni.jar'), targetDir)
            shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI.ini'), targetDir)

        # and now all drivers
        if self.osName != 'Android':
            binDriversDir = os.path.join(self.binDir, 'OpenNI2', 'Drivers')
            targetDriversDir = os.path.join(targetDir, 'OpenNI2', 'Drivers')
            os.makedirs(targetDriversDir)
        else:
            binDriversDir = self.binDir
            targetDriversDir = targetDir
            self.copySharedObject(binDriversDir, 'usb', targetDriversDir)

        self.copySharedObject(binDriversDir, 'OniFile', targetDriversDir)
        self.copySharedObject(binDriversDir, 'PS1080', targetDriversDir)
        self.copySharedObject(binDriversDir, 'PSLink', targetDriversDir)

        if self.osName != 'Android':
            shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI2', 'Drivers', 'PS1080.ini'), targetDriversDir)
            shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI2', 'Drivers', 'PSLink.ini'), targetDriversDir)
            shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI2', 'Drivers', 'OniFile.ini'), targetDriversDir)

        if self.osName == 'Windows':
            self.copySharedObject(binDriversDir, 'Kinect', targetDriversDir)

    def copySample(self, samplesDir, targetBinDir, name, isLibrary = False, isGL = False, isJava = False, sourceSamplesDir = None):
        if sourceSamplesDir is None:
            sourceSamplesDir = os.path.join(self.rootDir, 'Samples')

        sampleTargetDir = os.path.join(samplesDir, name)
        sampleSourceDir = os.path.join(sourceSamplesDir, name)

        if self.osName == 'Android':
            # remove the .Android in the end
            sampleTargetDir = os.path.splitext(sampleTargetDir)[0]

        # copy sources
        if self.osName == 'Android':
            shutil.copytree(os.path.join(sampleSourceDir, 'src'), os.path.join(sampleTargetDir, 'src'))
            shutil.copytree(os.path.join(sampleSourceDir, 'res'), os.path.join(sampleTargetDir, 'res'))
            if os.path.exists(os.path.join(sampleSourceDir, 'jni')):
                shutil.copytree(os.path.join(sampleSourceDir, 'jni'), os.path.join(sampleTargetDir, 'jni'))
            if os.path.exists(os.path.join(sampleSourceDir, 'assets')):
                shutil.copytree(os.path.join(sampleSourceDir, 'assets'), os.path.join(sampleTargetDir, 'assets'))
        else:
            for root, dirs, files in os.walk(sampleSourceDir):
                # take dir name without 'root' and append to target
                dst = os.path.join(sampleTargetDir, os.path.relpath(root, sampleSourceDir))
                for file in files:
                    if (isJava and file.endswith('.java')) or (not isJava and (file.endswith('.h') or file.endswith('.cpp'))):
                        if not os.path.exists(dst):
                            os.makedirs(dst)
                        shutil.copy(os.path.join(root, file), dst)

        # copy common header
        if not isJava and not isLibrary and self.osName != 'Android':
            shutil.copy(os.path.join(self.rootDir, 'Samples', 'Common', 'OniSampleUtilities.h'), sampleTargetDir)

        # copy GL headers
        if self.osName == 'Windows' and isGL:
            shutil.copytree(os.path.join(self.rootDir, 'ThirdParty', 'GL', 'GL'), os.path.join(sampleTargetDir, 'GL'))
            # and lib
            shutil.copy(os.path.join(self.rootDir, 'ThirdParty', 'GL', 'glut32.lib'), sampleTargetDir)
            shutil.copy(os.path.join(self.rootDir, 'ThirdParty', 'GL', 'glut64.lib'), sampleTargetDir)
            shutil.copy(os.path.join(self.rootDir, 'ThirdParty', 'GL', 'glut32.dll'), sampleTargetDir)
            shutil.copy(os.path.join(self.rootDir, 'ThirdParty', 'GL', 'glut64.dll'), sampleTargetDir)

        # and project file / makefile
        if self.osName == 'Windows':
            if isJava:
                shutil.copy(os.path.join(sampleSourceDir, 'Build.bat'), sampleTargetDir)
                shutil.copy(os.path.join(self.rootDir, 'ThirdParty', 'PSCommon', 'BuildSystem', 'BuildJavaWindows.py'), sampleTargetDir)
                # fix call
                buildFile = open(os.path.join(sampleTargetDir, 'Build.bat'))
                buildScript = buildFile.read()
                buildFile.close()

                buildScript = re.sub('..\\\\..\\\\ThirdParty\\\\PSCommon\\\\BuildSystem\\\\', '', buildScript)
                buildScript = re.sub('..\\\\..\\\\Bin', 'Bin', buildScript)

                buildFile = open(os.path.join(sampleTargetDir, 'Build.bat'), 'w')
                buildFile.write('@echo off\n')
                buildFile.write('IF "%1"=="x64" (\n')
                buildFile.write('\txcopy /D /S /F /Y "%OPENNI2_REDIST64%\\*" "Bin\\x64-Release\\"\n')
                buildFile.write(') ELSE (\n')
                buildFile.write('\txcopy /D /S /F /Y "%OPENNI2_REDIST%\\*" "Bin\\Win32-Release\\"\n')
                buildFile.write(')\n')
                buildFile.write(buildScript)
                buildFile.close()

            else:
                shutil.copy(os.path.join(sampleSourceDir, name + '.vcxproj'), sampleTargetDir)
                projFile = os.path.join(sampleTargetDir, name + '.vcxproj')
                #ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
                doc = xml.dom.minidom.parse(projFile)

                # remove OutDir and IntDir (make them default)
                for propertyGroup in doc.getElementsByTagName("PropertyGroup"):
                    if len(propertyGroup.getElementsByTagName("OutDir")) > 0:
                        propertyGroup.parentNode.removeChild(propertyGroup)

                for group in doc.getElementsByTagName("ItemDefinitionGroup"):
                    condAttr = group.getAttribute('Condition')
                    if condAttr.find('x64') != -1:
                        postfix = '64'
                        glPostfix = '64'
                    else:
                        postfix = ''
                        glPostfix = '32'

                    incDirs = group.getElementsByTagName('ClCompile')[0].getElementsByTagName('AdditionalIncludeDirectories')[0]
                    val = incDirs.firstChild.data

                    # fix GL include dir
                    val = re.sub('..\\\\..\\\\ThirdParty\\\\GL', r'.', val)
                    # fix Common include dir
                    val = re.sub('..\\\\Common', r'.', val)
                    # fix OpenNI include dir
                    val = re.sub('..\\\\..\\\\Include', '$(OPENNI2_INCLUDE' + postfix + ')', val)

                    incDirs.firstChild.data = val

                    # fix additional library directories
                    libDirs = group.getElementsByTagName('Link')[0].getElementsByTagName('AdditionalLibraryDirectories')[0]
                    val = libDirs.firstChild.data
                    val = re.sub('\$\(OutDir\)', '$(OutDir);$(OPENNI2_LIB' + postfix + ')', val)
                    libDirs.firstChild.data = val

                    # add post-build event to copy OpenNI redist
                    post = doc.createElement('PostBuildEvent')
                    cmd = 'xcopy /D /S /F /Y "$(OPENNI2_REDIST' + postfix + ')\*" "$(OutDir)"\n'
                    if isGL:
                        cmd += 'xcopy /D /F /Y "$(ProjectDir)\\glut' + glPostfix + '.dll" "$(OutDir)"\n'

                    cmdNode = doc.createElement('Command')
                    cmdNode.appendChild(doc.createTextNode(cmd))
                    post.appendChild(cmdNode)
                    group.appendChild(post)

                proj = open(projFile, 'w')
                proj.write(doc.toxml())
                proj.close()

        elif self.osName == 'Linux' or self.osName == 'Darwin':
            shutil.copy(os.path.join(sampleSourceDir, 'Makefile'), sampleTargetDir)
            shutil.copy(os.path.join(rootDir, 'ThirdParty', 'PSCommon', 'BuildSystem', 'CommonDefs.mak'), sampleTargetDir)
            shutil.copy(os.path.join(rootDir, 'ThirdParty', 'PSCommon', 'BuildSystem', 'CommonTargets.mak'), sampleTargetDir)
            shutil.copy(os.path.join(rootDir, 'ThirdParty', 'PSCommon', 'BuildSystem', 'Platform.x86'), sampleTargetDir)
            shutil.copy(os.path.join(rootDir, 'ThirdParty', 'PSCommon', 'BuildSystem', 'Platform.x64'), sampleTargetDir)
            shutil.copy(os.path.join(rootDir, 'ThirdParty', 'PSCommon', 'BuildSystem', 'Platform.Arm'), sampleTargetDir)
            if isJava:
                shutil.copy(os.path.join(rootDir, 'ThirdParty', 'PSCommon', 'BuildSystem', 'CommonJavaMakefile'), sampleTargetDir)
            else:
                shutil.copy(os.path.join(rootDir, 'ThirdParty', 'PSCommon', 'BuildSystem', 'CommonCppMakefile'), sampleTargetDir)

            # fix common makefiles path
            self.regxReplace('../../ThirdParty/PSCommon/BuildSystem/', '', os.path.join(sampleTargetDir, 'Makefile'))

            # fix BIN dir
            self.regxReplace('BIN_DIR = ../../Bin', 'BIN_DIR = Bin', os.path.join(sampleTargetDir, 'Makefile'))

            # fix include dirs and copy openni_redist
            add = r'''
ifndef OPENNI2_INCLUDE
    $(error OPENNI2_INCLUDE is not defined. Please define it or 'source' the OpenNIDevEnvironment file from the installation)
else ifndef OPENNI2_REDIST
    $(error OPENNI2_REDIST is not defined. Please define it or 'source' the OpenNIDevEnvironment file from the installation)
endif

INC_DIRS += $(OPENNI2_INCLUDE)

include \1

.PHONY: copy-redist
copy-redist:
	cp -R $(OPENNI2_REDIST)/* $(OUT_DIR)

$(OUTPUT_FILE): copy-redist
'''
            self.regxReplace(r'include (Common.*Makefile)', add, os.path.join(sampleTargetDir, 'Makefile'))
        elif self.osName == 'Android':
            shutil.copy(os.path.join(sampleSourceDir, '.classpath'), sampleTargetDir)
            shutil.copy(os.path.join(sampleSourceDir, '.project'), sampleTargetDir)
            shutil.copy(os.path.join(sampleSourceDir, 'AndroidManifest.xml'), sampleTargetDir)
            shutil.copy(os.path.join(sampleSourceDir, 'build.xml'), sampleTargetDir)
            shutil.copy(os.path.join(sampleSourceDir, 'proguard-project.txt'), sampleTargetDir)
            shutil.copy(os.path.join(sampleSourceDir, 'project.properties'), sampleTargetDir)
            # fix dependency
            self.regxReplace('../../../Wrappers/java', '../../OpenNIAndroidLibrary', os.path.join(sampleTargetDir, 'project.properties'))
            self.regxReplace('../../Wrappers/java', '../../OpenNIAndroidLibrary', os.path.join(sampleTargetDir, 'project.properties'))

        # and executable
        if self.osName == 'Android':
            apkName = glob.glob(os.path.join(sampleSourceDir, 'bin', '*-release.apk'))[0]
            realName = os.path.split(sampleTargetDir)[1]
            shutil.copy(apkName, os.path.join(targetBinDir, realName + '.apk'))
        elif isJava:
            splitName = os.path.splitext(name)
            # copy jar
            shutil.copy(os.path.join(self.binDir, 'org.openni.Samples.' + splitName[0] + '.jar'), targetBinDir)
            # and script
            if not isLibrary:
                if self.osName == 'Windows':
                    shutil.copy(os.path.join(self.binDir, 'org.openni.Samples.' + splitName[0] + '.bat'), targetBinDir)
                else:
                    shutil.copy(os.path.join(self.binDir, 'org.openni.Samples.' + splitName[0]), targetBinDir)
        elif isLibrary:
            self.copySharedObject(self.binDir, name, targetBinDir)
            if self.osName == 'Windows':
                shutil.copy(os.path.join(self.binDir, name + '.lib'), targetBinDir)
        else: # regular executable
            self.copyExecutable(self.binDir, name, targetBinDir)

    def copyTool(self, toolsDir, name, isGL = False):

        self.copyExecutable(self.binDir, name, toolsDir)

    def copyDocumentation(self, docDir):
        os.makedirs(docDir)
        if self.osName == 'Windows':
            shutil.copy(os.path.join(self.rootDir, 'Source', 'Documentation', 'cpp', 'OpenNI.chm'), docDir)
        else:
            shutil.copytree(os.path.join(self.rootDir, 'Source', 'Documentation', 'cpp'), os.path.join(docDir, 'cpp'))

        if self.osName == 'Android':
            shutil.copytree(os.path.join(self.rootDir, 'Wrappers', 'java', 'doc', 'gen'), os.path.join(docDir, 'java'))
        else:
            shutil.copytree(os.path.join(self.rootDir, 'Source', 'Documentation', 'java'), os.path.join(docDir, 'java'))

    def copyGLUT(self, targetDir):
        if self.osName == 'Windows':
            shutil.copy(os.path.join(self.rootDir, 'ThirdParty', 'GL', 'glut' + self.glutSuffix + '.dll'), targetDir)

    def copyAndroidLib(self, targetDir):
        os.makedirs(targetDir)
        shutil.copytree(os.path.join(self.rootDir, 'Wrappers', 'java', 'src'), os.path.join(targetDir, 'src'))
        shutil.copytree(os.path.join(self.rootDir, 'Wrappers', 'java', 'res'), os.path.join(targetDir, 'res'))
        shutil.copytree(os.path.join(self.rootDir, 'Wrappers', 'java', 'libs'), os.path.join(targetDir, 'libs'))
        os.makedirs(os.path.join(targetDir, 'bin'))
        shutil.copy(os.path.join(self.rootDir, 'Wrappers', 'java', 'bin', 'classes.jar'), os.path.join(targetDir, 'bin', 'org.openni.jar'))
        shutil.copy(os.path.join(self.rootDir, 'Wrappers', 'java', '.classpath'), targetDir)
        shutil.copy(os.path.join(self.rootDir, 'Wrappers', 'java', '.project'), targetDir)
        shutil.copy(os.path.join(self.rootDir, 'Wrappers', 'java', 'AndroidManifest.xml'), targetDir)
        shutil.copy(os.path.join(self.rootDir, 'Wrappers', 'java', 'build.xml'), targetDir)
        shutil.copy(os.path.join(self.rootDir, 'Wrappers', 'java', 'proguard-project.txt'), targetDir)
        shutil.copy(os.path.join(self.rootDir, 'Wrappers', 'java', 'project.properties'), targetDir)
        # remove redundant file
        os.remove(os.path.join(targetDir, 'res', '.gitignore'))

    def copyAssets(self, targetDir):
        os.makedirs(targetDir)
        shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI.ini'), targetDir)
        shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI2', 'Drivers', 'PS1080.ini'), targetDir)
        shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI2', 'Drivers', 'PSLink.ini'), targetDir)
        shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI2', 'Drivers', 'OniFile.ini'), targetDir)
        
    def createNativeMakefile(self, nativeDir, redistDir):
        nativeAndroidMk = open(os.path.join(nativeDir, 'Android.mk'), 'w')
        nativeAndroidMk.write('LOCAL_PATH := $(call my-dir)\n')
        
        libs = []
        for root, dirs, files in os.walk(redistDir):
            for file in files:
                if file.startswith('lib') and file.endswith('.so') and file != 'libOpenNI2.so':
                    moduleName = file[3:len(file)-3]
                    nativeAndroidMk.write('include $(CLEAR_VARS)\n')
                    libs.append(moduleName)
                    nativeAndroidMk.write('LOCAL_MODULE := ' + moduleName + '\n')
                    nativeAndroidMk.write('LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/' + file + '\n')
                    nativeAndroidMk.write('include $(PREBUILT_SHARED_LIBRARY)\n')
                    nativeAndroidMk.write('\n')
        
        # and now OpenNI itself
        nativeAndroidMk.write('include $(CLEAR_VARS)\n')
        nativeAndroidMk.write('LOCAL_MODULE := OpenNI2\n')
        nativeAndroidMk.write('LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libOpenNI2.so\n')
        nativeAndroidMk.write('LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include\n')
        nativeAndroidMk.write('LOCAL_SHARED_LIBRARIES := ' + ' '.join(libs) + '\n')
        nativeAndroidMk.write('include $(PREBUILT_SHARED_LIBRARY)\n')
        nativeAndroidMk.write('\n')

    def run(self):
        if os.path.exists(self.outDir):
            shutil.rmtree(self.outDir)
        os.makedirs(self.outDir)

        # Redist
        if self.osName == 'Android':
            nativeDir = os.path.join(self.outDir, 'Native', 'OpenNI2')
            redistDir = os.path.join(nativeDir, 'armeabi-v7a')
        else:
            redistDir = os.path.join(self.outDir, 'Redist')
        self.copyRedistFiles(redistDir)

        # Samples
        samplesDir = os.path.join(self.outDir, 'Samples')
        if self.osName != 'Android':
            samplesBinDir = os.path.join(samplesDir, 'Bin')
            self.copyRedistFiles(samplesBinDir)
            self.copyGLUT(samplesBinDir)
            self.copySample(samplesDir, samplesBinDir, 'SimpleRead')
            self.copySample(samplesDir, samplesBinDir, 'SimpleViewer', isGL = True)
            self.copySample(samplesDir, samplesBinDir, 'SimpleViewer.java', isJava = True)
            self.copySample(samplesDir, samplesBinDir, 'EventBasedRead')
            self.copySample(samplesDir, samplesBinDir, 'MultiDepthViewer', isGL = True)
            self.copySample(samplesDir, samplesBinDir, 'MultipleStreamRead')
            self.copySample(samplesDir, samplesBinDir, 'MWClosestPoint', isLibrary = True)
            self.copySample(samplesDir, samplesBinDir, 'MWClosestPointApp')
            self.copySample(samplesDir, samplesBinDir, 'ClosestPointViewer', isGL = True)
        else:
            samplesBinDir = os.path.join(samplesDir, 'Prebuilt')
            os.makedirs(samplesBinDir)
            self.copySample(samplesDir, samplesBinDir, 'SimpleRead.Android')
            self.copySample(samplesDir, samplesBinDir, 'SimpleViewer.Android')

        # Tools
        toolsDir = os.path.join(self.outDir, 'Tools')
        if self.osName != 'Android':
            self.copyRedistFiles(toolsDir)
            self.copyGLUT(toolsDir)
            self.copyTool(toolsDir, 'NiViewer', isGL = True)
            self.copyTool(toolsDir, 'PS1080Console')
            self.copyTool(toolsDir, 'PSLinkConsole')
        else:
            toolsSourceDir = os.path.join(self.rootDir, 'Source', 'Tools')
            toolsBinDir = os.path.join(toolsDir, 'Prebuilt')
            os.makedirs(toolsBinDir)
            self.copySample(toolsDir, toolsBinDir, 'NiViewer.Android', sourceSamplesDir = toolsSourceDir)

        # Documentation
        docDir = os.path.join(self.outDir, 'Documentation')
        self.copyDocumentation(docDir)

        # Include
        if self.osName == 'Android':
            incDir = os.path.join(nativeDir, 'include')
        else:
            incDir = os.path.join(self.outDir, 'Include')
        shutil.copytree(os.path.join(self.rootDir, 'Include'), incDir)

        # Android stuff
        if self.osName == 'Android':
            # Android native makefile
            self.createNativeMakefile(nativeDir, redistDir)

            # Android lib
            self.copyAndroidLib(os.path.join(self.outDir, 'OpenNIAndroidLibrary'))
            self.copyAssets(os.path.join(self.outDir, 'Assets', 'openni'))
            

        # Release notes and change log
        shutil.copy(os.path.join(self.rootDir, 'ReleaseNotes.txt'), self.outDir)
        shutil.copy(os.path.join(self.rootDir, 'CHANGES.txt'), self.outDir)

        # Licenses
        shutil.copy(os.path.join(self.rootDir, 'NOTICE'), self.outDir)
        shutil.copy(os.path.join(self.rootDir, 'LICENSE'), self.outDir)

        if self.osName == 'Windows':
            # Driver
            shutil.copytree(os.path.join(self.rootDir, 'ThirdParty', 'PSCommon', 'XnLib', 'Driver', 'Win32', 'Bin'), os.path.join(self.outDir, 'Driver'))

            # Library
            libDir = os.path.join(self.outDir, 'Lib')
            os.makedirs(libDir)
            shutil.copy(os.path.join(self.binDir, 'OpenNI2.lib'), libDir)
        elif self.osName != 'Android':
            # install script
            shutil.copy(os.path.join(self.rootDir, 'Packaging', 'Linux', 'install.sh'), self.outDir)
            shutil.copy(os.path.join(self.rootDir, 'Packaging', 'Linux', 'primesense-usb.rules'), self.outDir)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: ' + sys.argv[0] + ' <OutDir> <x86|x64|Arm|Android>')
        exit(1)

    rootDir = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), '..'))
    arch = sys.argv[2]
    osName = platform.system()
    if sys.argv[2] == 'Android':
        arch = 'Arm'
        osName = 'Android'
    harvest = Harvest(rootDir, sys.argv[1], arch, osName)
    harvest.run()
