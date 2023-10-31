# midas_publisher
Install instructions:
1. cd environment_setup
2. ./detect_environment.sh
3. cat environment_variables.txt
4. Ensure each environment_variable points to the correct directory
5. source ./setup_environment.sh
6. cd ..
7. mkdir build
8. cd build
9. cmake3 ..
10. make install -j$(nproc)
11. cd ..
12. ./publisher
