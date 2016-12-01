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
#
import sys
import os
import re
import stat
from datetime import date

VERSION_MAJOR = 2
VERSION_MINOR = 2
VERSION_MAINTENANCE = 0
VERSION_BUILD = 33

def getVersionString():
    return str(VERSION_MAJOR) + "." + str(VERSION_MINOR) + "." + str(VERSION_MAINTENANCE) + "." + str(VERSION_BUILD)
    
def getVersionName():
    if VERSION_MAINTENANCE != 0:
        return str(VERSION_MAJOR) + "." + str(VERSION_MINOR) + "." + str(VERSION_MAINTENANCE)
    elif VERSION_MINOR != 0:
        return str(VERSION_MAJOR) + "." + str(VERSION_MINOR)
    else:
        return str(VERSION_MAJOR)

def update():
    if VERSION_MAJOR > 9:
        print( "Illegal major version")
        sys.exit()

    if (VERSION_MINOR > 99):
        print ("Illegal minor version")
        sys.exit()

    if (VERSION_MAINTENANCE > 99):
        print ("Illegal maintenance version")
        sys.exit()

    if (VERSION_BUILD > 9999):
        print ("Illegal build version")
        sys.exit()

    print("Going to update files to version: " + getVersionString())

    update_self_defs("./UpdateVersion.py")
    update_src_ver_defs("../Include/OniVersion.h")
    update_wix_include("Install/Includes/Variables.wxi")
    update_wix_project("Install/Install.wixproj")
    update_doxygen("../Source/Documentation/Doxyfile")
    update_release_notes("../ReleaseNotes.txt")

    print ("\n*** Done ***")

def regx_replace(findStr,repStr,filePath):
    "replaces all findStr by repStr in file filePath using regualr expression"
    findStrRegx = re.compile(findStr)
    tempName=filePath+'~~~'
    fileMode = os.stat(filePath).st_mode
    os.chmod(filePath, fileMode | stat.S_IWRITE)
    input = open(filePath)
    output = open(tempName,'w')
    for s in input:
        output.write(findStrRegx.sub(repStr,s))
    output.close()
    input.close()
    os.remove(filePath)
    os.rename(tempName,filePath)

def update_self_defs (filePath):
    print(( "Updating self version defines: " + filePath))
    regx_replace("VERSION_MAJOR = \d+\n", "VERSION_MAJOR = " + str(VERSION_MAJOR) + "\n", filePath)
    regx_replace("VERSION_MINOR = \d+\n", "VERSION_MINOR = " + str(VERSION_MINOR) + "\n", filePath)
    regx_replace("VERSION_MAINTENANCE = \d+\n", "VERSION_MAINTENANCE = " + str(VERSION_MAINTENANCE) + "\n", filePath)
    regx_replace("VERSION_BUILD = \d+\n", "VERSION_BUILD = " + str(VERSION_BUILD) + "\n", filePath)

def update_src_ver_defs (filePath):
    print(( "Updating source version defines: " + filePath))
    regx_replace("#define ONI_VERSION_MAJOR[ \t](.*)", "#define ONI_VERSION_MAJOR\t" + str(VERSION_MAJOR), filePath)
    regx_replace("#define ONI_VERSION_MINOR[ \t](.*)", "#define ONI_VERSION_MINOR\t" + str(VERSION_MINOR), filePath)
    regx_replace("#define ONI_VERSION_MAINTENANCE[ \t](.*)", "#define ONI_VERSION_MAINTENANCE\t" + str(VERSION_MAINTENANCE), filePath)
    regx_replace("#define ONI_VERSION_BUILD[ \t](.*)", "#define ONI_VERSION_BUILD\t" + str(VERSION_BUILD), filePath)

def update_wix_include (filePath):
    print (("Updating wix include: " + filePath))
    regx_replace("define MajorVersion=(.*)", "define MajorVersion=" + str(VERSION_MAJOR) + "?>", filePath)
    regx_replace("define MinorVersion=(.*)", "define MinorVersion=" + str(VERSION_MINOR) + "?>", filePath)
    regx_replace("define MaintenanceVersion=(.*)", "define MaintenanceVersion=" + str(VERSION_MAINTENANCE) + "?>", filePath)
    regx_replace("define BuildVersion=(.*)", "define BuildVersion=" + str(VERSION_BUILD) + "?>", filePath)
    regx_replace("define VersionName=(.*)", "define VersionName=\"" + getVersionName() + "\"?>", filePath)

def update_wix_project (filePath):
    print (("Updating wix project: " + filePath))
    regx_replace("<OutputName>(.*)</OutputName>", "<OutputName>OpenNI-Windows-$(Platform)-" + getVersionName() + "</OutputName>", filePath)

def update_doxygen (filePath):
    print (("Updating doxygen: " + filePath))
    regx_replace("PROJECT_NAME\s*=\s*\"OpenNI (\d+)\.(\d+)\.(\d+)\"", "PROJECT_NAME = \"OpenNI " + getVersionName() + "\"", filePath)

def update_release_notes (filePath):
    print (("Updating release notes: " + filePath))

    tempName = filePath + '~~~'
    os.system("attrib -r " + filePath)
    input = open(filePath)
    output = open(tempName, 'w')
    lines = input.readlines()
    input.close()
    today = date.today()

    lines[0] = 'OpenNI ' + str(VERSION_MAJOR) + '.' + str(VERSION_MINOR) + '.' + str(VERSION_MAINTENANCE) + ' Build ' + str(VERSION_BUILD) + '\n'
    lines[1] = today.strftime('%B ') + str(today.day) + ' ' + str(today.year) + '\n'
    
    for s in lines:
        output.write(s)
    output.close()
    os.remove(filePath)
    os.rename(tempName,filePath)        
    
if __name__ == '__main__':
    if len(sys.argv) == 5:
        VERSION_MAJOR = int(sys.argv[1])
        VERSION_MINOR = int(sys.argv[2])
        VERSION_MAINTENANCE = int(sys.argv[3])
        VERSION_BUILD = int(sys.argv[4])

    update()
