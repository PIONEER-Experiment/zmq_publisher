# midas_publisher

This version of the publisher was made in a hurry and contains a lot of hardcoded features. I would not recommend using this branch.

Install instructions:
1. cd environment_setup
2. ./detect_environment.sh
3. cat environment_variables.txt
  
[Ensure each environment_variable points to the correct directory]

4. source ./setup_environment.sh
5. cd ..
6. mkdir build
7. cd build
8. cmake3 ..
9. make install -j$(nproc)
10. cd ..
11. vi  config.json

[Edit the config to match your specifications usings your favorite text editor. In particular, 
"detector-mapping-file": "/home/installation_testing/packages/unpacking/python/detector_mapping.json"
needs to be set to a valid detector mapping. The unpacking library has one in unpacking/python/detector_mapping.json]

12. ./publisher
