// nrc.cpp
/*
neoGFX Resource Compiler
Copyright(C) 2016 Leigh Johnston

This program is free software: you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <neolib/neolib.hpp>
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <neolib/json.hpp>

struct invalid_file : std::runtime_error 
{ 
    invalid_file() : std::runtime_error("Not a valid neoGFX resource meta file (.nrc)!") {} 
    invalid_file(const std::string& aReason) : std::runtime_error("Not a valid neoGFX resource meta file (.nrc), " + aReason + "!") {}
};
struct failed_to_read_resource_file : std::runtime_error
{
    failed_to_read_resource_file(const std::string& aPath) : std::runtime_error("Failed to read resource file '" + aPath + "'!") {}
};
struct bad_usage : std::runtime_error { bad_usage() : std::runtime_error("Bad usage") {} };
struct not_yet_implemented : std::runtime_error { not_yet_implemented() : std::runtime_error("Not yet implemented") {} };

void parse_resource(const boost::filesystem::path& aInputFilename, const neolib::fjson_value& aItem, std::ofstream& aOutput)
{
    typedef neolib::fjson_string symbol_t;
    std::vector<std::string> resourcePaths;
    uint32_t resourceIndex = 0;
    auto const& resource = aItem.as<neolib::fjson_object>();
    auto const& resourcePrefix = resource.has("prefix") ? resource.at("prefix").text() : "";

    auto const& resourceRef = resource.has("ref") ? resource.at("ref").text() : "";
    auto symbol = resourcePrefix;
    if (!symbol.empty() && !resourceRef.empty())
        symbol += "_";
    symbol += resourceRef;

    aOutput << "namespace nrc" << std::endl << "{" << std::endl;
    aOutput << "namespace" << std::endl << "{" << std::endl;

    for (const auto& resourceItem : aItem)
    {
        auto process_file = [&](const neolib::fjson_string& aFile)
        {
            std::cout << "Processing " << aFile << "..." << std::endl;
            resourcePaths.push_back((!resourcePrefix.empty() ? resourcePrefix + "/" : "") + aFile);
            std::string resourcePath = boost::filesystem::path(aInputFilename).parent_path().string();
            if (!resourcePath.empty())
                resourcePath += "/";
            resourcePath += aFile;
            std::ifstream resourceFile(resourcePath, std::ios_base::in | std::ios_base::binary);
            aOutput << "\tconst unsigned char resource_" << resourceIndex << "_data[] =" << std::endl << "\t{" << std::endl;
            const std::size_t kBufferSize = 32;
            bool doneSome = false;
            unsigned char buffer[kBufferSize];
            while (resourceFile)
            {
                resourceFile.read(reinterpret_cast<char*>(buffer), kBufferSize);
                std::streamsize amount = resourceFile.gcount();
                if (amount != 0)
                {
                    if (doneSome)
                        aOutput << ", " << std::endl;
                    aOutput << "\t\t";
                    for (std::size_t j = 0; j != amount;)
                    {
                        aOutput << "0x";
                        aOutput.width(2);
                        aOutput.fill('0');
                        aOutput << std::hex << std::uppercase << static_cast<unsigned int>(buffer[j]);
                        if (++j != amount)
                            aOutput << ", ";
                    }
                    doneSome = true;
                }
                else
                {
                    aOutput << std::endl;
                    break;
                }
            }
            if (resourceFile.fail() && !resourceFile.eof())
                throw failed_to_read_resource_file(resourcePath);
            aOutput << "\t};" << std::endl;
            ++resourceIndex;
        };

        if (resourceItem.name() == "file")
            process_file(resourceItem.text());
        else if (resourceItem.name() == "files")
            for (auto const& fileItem : resourceItem)
                process_file(fileItem.text());
        else 
            continue;
    }

    aOutput << "\n\tstruct register_data" << std::endl << "\t{" << std::endl;
    aOutput << "\t\tregister_data()" << std::endl << "\t\t{" << std::endl;
    for (std::size_t i = 0; i < resourcePaths.size(); ++i)
    {
        aOutput << "\t\t\tneogfx::resource_manager::instance().add_module_resource("
            << "\":/" << resourcePaths[i] << "\", " << "resource_" << i << "_data, " << "sizeof(resource_" << i << "_data)"
            << ");" << std::endl;
    }

    aOutput << "\t\t}" << std::endl;

    aOutput << "\t} " << symbol << ";" << std::endl;

    aOutput << "}" << std::endl << "}" << std::endl << std::endl;

    aOutput << "extern \"C\" void* nrc_" << symbol << " = &nrc::" << symbol << ";" << std::endl << std::endl;
}

void parse_ui(const neolib::fjson_value& aItem, std::ofstream& aOutput)
{
    auto const& ui = aItem.as<neolib::fjson_object>();
    auto const& ns = ui.has("namespace") ? ui.at("namespace").text() + "::ui" : "ui";
    aOutput << "namespace " << ns << std::endl << "{" << std::endl;
    aOutput << "}" << std::endl;
}

int main(int argc, char* argv[])
{
    std::cout << "nrc neoGFX resource compiler" << std::endl;
    std::cout << "Copyright (c) 2016 Leigh Johnston" << std::endl << std::endl;
    std::vector<std::string> options;
    std::vector<std::string> files;
    for (int a = 1; a < argc; ++a)
        if (argv[a][0] == '-')
            options.push_back(argv[a]);
        else
            files.push_back(argv[a]);
    try
    {
        if (files.size() < 1 || files.size() > 2)
        {
            throw bad_usage();
        }

        boost::filesystem::path const inputFileName{ files[0] };
        std::cout << "Resource meta file: " << inputFileName << std::endl;
        neolib::fjson const input{ inputFileName.string() };
        if (!input.has_root())
            throw invalid_file("bad root node");

        std::string const outputDirectory{ files.size() > 1 ? files[1] : boost::filesystem::current_path().string() };
         
        std::string resourceFileName; 
        std::string uiFileName;
        if (options.empty() || options[0] == "-embed")
        {
            resourceFileName = inputFileName.filename().stem().string() + ".nrc.cpp";
            uiFileName = inputFileName.filename().stem().string() + ".ui.hpp";
        }
        else if (options[0] == "-archive")
            throw not_yet_implemented();
        else
            throw bad_usage();

        if (options.empty() || options[0] == "-embed")
        {
            std::optional<std::ofstream> resourceOutput;
            std::optional<std::ofstream> uiOutput;
            for (const auto& item : input.root())
            {
                if (item.name() == "resource")
                {
                    if (resourceOutput == std::nullopt)
                    {
                        auto resourceOutputPath = outputDirectory + "/" + resourceFileName;
                        std::cout << "Creating " << resourceOutputPath << "..." << std::endl;
                        resourceOutput.emplace(resourceOutputPath);
                        *resourceOutput << "// This is a automatically generated file, do not edit!" << std::endl << std::endl;
                        *resourceOutput << "#include <neogfx/app/resource_manager.hpp>" << std::endl << std::endl;
                    }
                    parse_resource(inputFileName, item, *resourceOutput);
                }
                else if (item.name() == "ui")
                {
                    if (uiOutput == std::nullopt)
                    {
                        auto uiOutputPath = outputDirectory + "/" + uiFileName;
                        std::cout << "Creating " << uiOutputPath << "..." << std::endl;
                        uiOutput.emplace(uiOutputPath);
                        *uiOutput << "// This is a automatically generated file, do not edit!" << std::endl << std::endl;
                    }
                    parse_ui(item, *uiOutput);
                }
            }
        }
    }
    catch (const bad_usage&)
    {
        std::cerr << "Usage: " << argv[0] << " [-embed|-archive] <input path> [<output directory>]" << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}

