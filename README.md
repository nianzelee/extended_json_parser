# extended_json_parser

## parse an extended json file and generate a regular json file

## how to use

### compilation

1. Download this archive
2. Downlaod json.hpp from [Niels Lohmann's JSON for modern C++ repository](https://github.com/nlohmann/json/blob/develop/include/nlohmann/json.hpp)
3. Put the json.hpp in `src/nlohmann`
4. `make`

### using

`exj_parser in.json [inter.json] out.json`, where `in.json` is an extended JSON file, the result is stored in `out.json`, and `inter.json` is an optional intermediate extended JSON file, in which `repeat` and `choice` keywords have been expanded.

## supported keywords

1. uniform a b
2. repeat n
3. choice
