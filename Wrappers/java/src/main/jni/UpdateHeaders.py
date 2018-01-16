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

import platform
import subprocess
import os
import re
import glob
import sys

if len(sys.argv) < 2 or sys.argv[1] in ('-h', '--help') or not sys.argv[1] in('android', 'java'):
    print('usage: ' + sys.argv[0] + ' <android|java>')
    sys.exit(1)

type = sys.argv[1]

# list all classes
java_classes = ['org.openni.NativeMethods']
android_classes = ['org.openni.android.OpenNIView']

classes = java_classes
if type == 'android':
    classes += android_classes

# decide where class files can be found
if type == 'android':
    class_dir = "../bin/classes"
else:
    class_dir = "../../../Bin/Intermediate/x64-Release/org.openni"

# run javah to create the header files
if platform.system() == 'Windows':
    JAVAH = os.path.join(os.environ['JAVA_HOME'], 'bin', 'javah')
    if type == 'android':
        class_dir += ";" + os.environ['ANDROID_HOME'] + "\\platforms\\android-12\\android.jar"
else:    
    JAVAH = 'javah'

cmd = [JAVAH, '-classpath', class_dir] + classes
print(cmd)
subprocess.check_call(cmd)

# now create the methods.inl file
result = open("methods.inl", "w")
register_all = ""

for classname in classes:
    prefix = ""
    postfix = ""
    if classname in android_classes:
        prefix = "#ifdef ANDROID\n"
        postfix = "#endif\n"

    for filename in glob.glob(classname.replace('.', '_') + '*.h'):
        with open(filename) as file:
            cont = file.read()

        class_name_underscore = os.path.splitext(filename)[0]
        class_name = class_name_underscore.replace('_', '/')

        methods = ""
        while True:
            match = re.search("Method:\s*(\w*)", cont)
            if match is None:
                break
            method_name = match.group(1)

            match = re.search("Signature:\s*([\w\(\)\[;/]*)", cont)
            if match is None:
                break
            signature = match.group(1)

            match = re.search("JNIEXPORT.*JNICALL (\w*)", cont)
            if match is None:
                break
            method = match.group(1)

            methods += '\t\t{ "' + method_name + '", "' + signature + '", (void*)&' + method + ' },\n'
            cont = cont[match.end():]
            
        if len(methods) > 0:
            template = """
static int register_%CLASSNAMEUNDERSCORE%(JNIEnv* env)
{
    static JNINativeMethod methods[] =
    {
%METHODS%
    };
    static int methodsCount = sizeof(methods)/sizeof(methods[0]);
    jclass cls = env->FindClass("%CLASSNAME%");
    if (cls == NULL) {
        LOGE("Native registration unable to find class '%CLASSNAME%'");
        return JNI_FALSE;
    }
    if (env->RegisterNatives(cls, methods, methodsCount) < 0) {
        LOGE("RegisterNatives failed for '%CLASSNAME%'");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}
"""
            template = re.sub("%CLASSNAMEUNDERSCORE%", class_name_underscore, template)
            template = re.sub("%CLASSNAME%", class_name, template)
            template = re.sub("%METHODS%", methods, template)

            result.write(prefix)
            result.write('#include "' + filename + '"\n')
            result.write(template);
            result.write(postfix)
            result.write('\n')

            register_all += prefix + "\tif (register_" + class_name_underscore + "(env) != JNI_TRUE) return JNI_FALSE;\n" + postfix

result.write("static int registerNatives(JNIEnv* env)\n")
result.write("{\n")
result.write(register_all)
result.write("\treturn JNI_TRUE;\n")
result.write("}\n")
result.close()

