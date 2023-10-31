# midas_publisher
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
121. ./publisher
