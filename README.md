# http-gameserver


## About
todo

## Dependencies
В основе проекте использовалась библиотека boost 1.78.0. Для обработки сетевых взаимодействий был использован [boost.asio](https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio.html) и 
[boost.beast](https://www.boost.org/doc/libs/1_78_0/libs/beast/doc/html/index.html). Для сбора и ротации лог-файлов использовался [boost.log](https://www.boost.org/doc/libs/1_78_0/libs/log/doc/html/index.html).
Передача и сериализация формата json - [boost.json](https://www.boost.org/doc/libs/1_78_0/libs/json/doc/html/index.html).  
Для установки библиотеки можно использовать подгрузчик conan и представленный conanfile.txt.

## Building
### Linux

При сборке под Linux обязательно указываются флаги:
* `-s compiler.libcxx=libstdc++11`
* `-s build_type=???`

Вот пример конфигурирования для Release и Debug:
```
# mkdir -p build-release 
# cd build-release
# conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11
# cmake .. -DCMAKE_BUILD_TYPE=Release
# cd ..

# mkdir -p build-debug
# cd build-debug
# conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11
# cmake .. -DCMAKE_BUILD_TYPE=Debug
# cd ..

```

### Windows

Нужно выполнить два шага:
1. В conanfile.txt обязательно используем `cmake_multi`.
2. В CMakeLists.txt: `include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)`.

После этого можно запустить подобный снипет:

```
# mkdir build 
# cd build
# conan install .. --build=missing -s build_type=Debug
# conan install .. --build=missing -s build_type=Release
# conan install .. --build=missing -s build_type=RelWithDebInfo
# conan install .. --build=missing -s build_type=MinSizeRel
# cmake ..
```

В таком случае будут собираться все конфигурации (что не быстро). Можно сэкономить время, оставив только нужные.

Запускать сборку нужно только через родной cmd. В других консолях иногда возникают проблемы.

## Running

В папке `build/bin` или `build/Release`/`build/Debug` выполнить команду
```sh
./game_server -c ../../data/config.json -w ../../static/ --log-file <LOG_FILE_PATH>
              -t <TICK_PERIOD_IN_MS> --state-file <STATE_FILE_PATH> --save-state-period <SAVE_PERIOD_IN_MS>
              --randomize-spawn-points
```
Здесь:
* `-c ../../data/config.json` - путь к файлу с конфигурацие игры (обязательный параметр)
* `-w ../../static/` - путь к директории со статическими файлами (обязательный параметр)
* `--log-file <LOG_FILE_PATH>` - путь к файлу с логами (обязательный параметр)
* `-t <TICK_PERIOD_IN_MS>` - период автотика (обязательный параметр)
* `--state-file <STATE_FILE_PATH>` - путь к файлу с последнем игровым состоянием. При использовании данного параметра при закрытии сервера последнее состояние автоматически сохранится (необязательный параметр)
* `--save-state-period <SAVE_PERIOD_IN_MS>` - период автосохранения игрового состояния (необязательный параметр)
* `--randomize-spawn-points` - при использовании данного параметра игроки появляются в случайно точке карты (необязательный параметр)


После этого можно открыть в браузере: http://127.0.0.1:8080/index.html - Меню игры
