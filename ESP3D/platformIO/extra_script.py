from os.path import join, isfile
import re
Import("env")
# access to global construction environment
ROOT_DIR = env['PROJECT_DIR']
# configuration file
configuration_file = join(ROOT_DIR, "esp3d", "configuration.h")
print("Check if need to add some library to path")
if isfile(configuration_file):
    fh = open(configuration_file, 'r')
    entry = None
    for line in fh:
        pattern =r'^\s*#\s*define\s+SD_DEVICE\s+ESP_SDFAT2'
        entry = re.search(pattern, line)
        if entry:
            print("Need to add some SD FAT library to path")
            if (env["PIOPLATFORM"] == "espressif8266"):
                lib_ignore = env.GetProjectOption("lib_ignore")
                lib_ignore.append("SD(esp8266)")
                lib_ignore.append("SD")
                lib_ignore.append("SDFS")
                print("Ignore libs:", lib_ignore)
                env.GetProjectConfig().set(
                    "env:" + env["PIOENV"], "lib_ignore", lib_ignore)
                print("Add ESP8266SDFat2 library to path")
                env["LIBSOURCE_DIRS"].append(
                        "extra-libraries/ESP8266")
            else:
                print("Add SDFat2 library to path")
                env["LIBSOURCE_DIRS"].append("extra-libraries/ESP32")
            break
    fh.close()
    if entry is None:
        print("No need to add any extra library")
else:
    print("No configuration.h file found")
print(env["LIBSOURCE_DIRS"])
