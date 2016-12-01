#!/usr/bin/python

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
import re
import sys
import shutil
import platform
import stat
import xml.dom.minidom

class Harvest:
    def __init__(self, rootDir, outDir, arch):
        self.rootDir = rootDir
        self.outDir = outDir
        self.arch = arch
        self.osName = platform.system()
        self.binDir = os.path.join(rootDir, 'Bin', arch + '-Release')
        self.platformSuffix = ''
        self.glutSuffix = '32'
        
        if self.osName == 'Windows':
            if arch == 'x86':
                self.binDir = os.path.join(rootDir, 'Bin', 'Win32-Release')
            elif arch == 'x64':
                self.platformSuffix = '64'
                self.glutSuffix = '64'

    def copySharedObject(self, sourceDir, name, targetDir):
        if self.osName == 'Windows':
            shutil.copy(os.path.join(sourceDir, name + '.dll'), targetDir)
            shutil.copy(os.path.join(sourceDir, name + '.pdb'), targetDir)
        elif self.osName == 'Linux':
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
        "replaces all findStr by repStr in file filePath using regualr expression"
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
        shutil.copy(os.path.join(self.binDir, 'org.openni.jar'), targetDir)
        shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI.ini'), targetDir)
        # and now all drivers
        binDriversDir = os.path.join(self.binDir, 'OpenNI2', 'Drivers')
        targetDriversDir = os.path.join(targetDir, 'OpenNI2', 'Drivers')
        os.makedirs(targetDriversDir)
        self.copySharedObject(binDriversDir, 'OniFile', targetDriversDir)
        self.copySharedObject(binDriversDir, 'PS1080', targetDriversDir)
        self.copySharedObject(binDriversDir, 'PSLink', targetDriversDir)
        shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI2', 'Drivers', 'PS1080.ini'), targetDriversDir)
        shutil.copy(os.path.join(self.rootDir, 'Config', 'OpenNI2', 'Drivers', 'PSLink.ini'), targetDriversDir)
        if self.osName == 'Windows':
            self.copySharedObject(binDriversDir, 'Kinect', targetDriversDir)
        
    def copySample(self, samplesDir, name, isLibrary = False, isGL = False, isJava = False):
        if self.arch == 'Arm' and isGL:
            return
            
        sampleTargetDir = os.path.join(samplesDir, name)
        sampleSourceDir = os.path.join(self.rootDir, 'Samples', name)

        # copy sources
        for root, dirs, files in os.walk(sampleSourceDir):
            # take dir name without 'root' and append to target
            dst = os.path.join(samplesDir, name, os.path.relpath(root, sampleSourceDir))
            #print dst
            for file in files:
                if (isJava and file.endswith('.java')) or (not isJava and (file.endswith('.h') or file.endswith('.cpp'))):
                    if not os.path.exists(dst):
                        os.makedirs(dst)
                    shutil.copy(os.path.join(root, file), dst)
                    
        # copy common header
        if not isJava and not isLibrary:
            shutil.copy(os.path.join(self.rootDir, 'Samples', 'Common', 'OniSampleUtilities.h'), sampleTargetDir)
            
        # copy GL headers
        if self.osName == 'Windows' and isGL:
            shutil.copytree(os.path.join(self.rootDir, 'ThirdParty', 'GL', 'GL'), os.path.join(sampleTargetDir, 'GL'))
            shutil.copytree(os.path.join(self.rootDir, 'ThirdParty', 'GL', 'glh'), os.path.join(sampleTargetDir, 'glh'))
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

        # and executable
        if isJava:
            splitName = os.path.splitext(name)
            # copy jar
            shutil.copy(os.path.join(self.binDir, 'org.openni.Samples.' + splitName[0] + '.jar'), os.path.join(samplesDir, 'Bin'))
            # and script
            if not isLibrary:
                if self.osName == 'Windows':
                    shutil.copy(os.path.join(self.binDir, 'org.openni.Samples.' + splitName[0] + '.bat'), os.path.join(samplesDir, 'Bin'))
                else:
                    shutil.copy(os.path.join(self.binDir, 'org.openni.Samples.' + splitName[0]), os.path.join(samplesDir, 'Bin'))
        elif isLibrary:
            self.copySharedObject(self.binDir, name, os.path.join(samplesDir, 'Bin'))
            if self.osName == 'Windows':
                shutil.copy(os.path.join(self.binDir, name + '.lib'), os.path.join(samplesDir, 'Bin'))
        else: # regular executable
            self.copyExecutable(self.binDir, name, os.path.join(samplesDir, 'Bin'))
        
    def copyTool(self, toolsDir, name, isGL = False):
        if self.arch == 'Arm' and isGL:
            return
            
        self.copyExecutable(self.binDir, name, toolsDir)

    def copyDocumentation(self, docDir):
        if self.osName == 'Windows':
            os.makedirs(docDir)
            shutil.copy(os.path.join(self.rootDir, 'Source', 'Documentation', 'html', 'OpenNI.chm'), docDir)
        else:
            shutil.copytree(os.path.join(self.rootDir, 'Source', 'Documentation', 'html'), docDir)
            
        shutil.copytree(os.path.join(self.rootDir, 'Source', 'Documentation', 'java'), os.path.join(docDir, 'java'))
            
    def copyGLUT(self, targetDir):
        if self.osName == 'Windows':
            shutil.copy(os.path.join(rootDir, 'ThirdParty', 'GL', 'glut' + self.glutSuffix + '.dll'), targetDir)
        
    def run(self):
        if os.path.exists(self.outDir):
            shutil.rmtree(self.outDir)
        os.makedirs(self.outDir)
        
        # Redist
        self.copyRedistFiles(os.path.join(self.outDir, 'Redist'))
        
        # Samples
        samplesDir = os.path.join(self.outDir, 'Samples')
        self.copyRedistFiles(os.path.join(samplesDir, 'Bin'))
        self.copyGLUT(os.path.join(samplesDir, 'Bin'))
        self.copySample(samplesDir, 'SimpleRead')
        self.copySample(samplesDir, 'SimpleViewer', isGL = True)
        self.copySample(samplesDir, 'SimpleViewer.java', isJava = True)
        self.copySample(samplesDir, 'EventBasedRead')
        self.copySample(samplesDir, 'MultiDepthViewer', isGL = True)
        self.copySample(samplesDir, 'MultipleStreamRead')
        self.copySample(samplesDir, 'MWClosestPoint', isLibrary = True)
        self.copySample(samplesDir, 'MWClosestPointApp')
        self.copySample(samplesDir, 'ClosestPointViewer', isGL = True)
        
        # Tools
        toolsDir = os.path.join(self.outDir, 'Tools')
        self.copyRedistFiles(toolsDir)
        self.copyGLUT(toolsDir)
        self.copyTool(toolsDir, 'NiViewer', isGL = True)
        self.copyTool(toolsDir, 'PS1080Console')
        self.copyTool(toolsDir, 'PSLinkConsole')
        
        # Documentation
        docDir = os.path.join(self.outDir, 'Documentation')
        self.copyDocumentation(docDir)
        
        # Include
        shutil.copytree(os.path.join(rootDir, 'Include'), os.path.join(self.outDir, 'Include'))
        
        # Licenses
        shutil.copy(os.path.join(rootDir, 'NOTICE'), self.outDir)
        shutil.copy(os.path.join(rootDir, 'LICENSE'), self.outDir)
        
        if self.osName == 'Windows':
            # Driver
            shutil.copytree(os.path.join(rootDir, 'ThirdParty', 'PSCommon', 'XnLib', 'Driver', 'Win32', 'Bin'), os.path.join(self.outDir, 'Driver'))
            
            # Library
            libDir = os.path.join(self.outDir, 'Lib')
            os.makedirs(libDir)
            shutil.copy(os.path.join(self.binDir, 'OpenNI2.lib'), libDir)
        else:
            # install script
            shutil.copy(os.path.join(self.rootDir, 'Packaging', 'Linux', 'install.sh'), self.outDir)
            shutil.copy(os.path.join(self.rootDir, 'Packaging', 'Linux', 'primesense-usb.rules'), self.outDir)

if len(sys.argv) < 3:
    print('Usage: ' + sys.argv[0] + ' <OutDir> <x86|x64|Arm>')
    exit(1)
    
rootDir = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), '..'))
harvest = Harvest(rootDir, sys.argv[1], sys.argv[2])
harvest.run()
