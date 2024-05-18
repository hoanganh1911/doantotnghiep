# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c 
	     WiFiAP/WiFiAP.c 
	     WSServer/WSServer.c  
	     nvs_storage/nvs_storage.c
    INCLUDE_DIRS . 
    			 WiFiAP/ 
    			 WSServer/
    			 nvs_storage/      
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
    EMBED_FILES websource/index.html
			    websource/style.css
			    websource/script.js
)
