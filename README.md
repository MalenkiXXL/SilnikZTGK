Jakby ci nie dzialala jakas biblioteka uzyj sobie vcpkg (poza glad ktory jest zainstalowany w repo w sumie)

cd C:\ścieżka\do\twojego\vcpkg
.\vcpkg install glfw3:x64-windows glm:x64-windows
.\vcpkg integrate install


Jakbys nie mial vcpkg
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
