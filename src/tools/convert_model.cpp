#include "common/iman.h"
#include "common/logger.h"
#include "graphics/engine/modelfile.h"

#include <iostream>
#include <map>


bool EndsWith(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}


struct Args
{
    bool usage;
    bool dumpInfo;
    bool mirror;
    std::string inputFile;
    std::string outputFile;
    std::string inputFormat;
    std::string outputFormat;

    Args()
    {
        usage = false;
        dumpInfo = false;
        mirror = false;
    }
};

Args ARGS;

void PrintUsage(const std::string& program)
{
    std::cerr << "Colobot model converter" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage:" << std::endl;
    std::cerr << std::endl;
    std::cerr << " Convert files:" << std::endl;
    std::cerr << "   " << program << " -i input_file -if input_format -o output_file -of output_format [-m]" << std::endl;
    std::cerr << " -m => mirror" << std::endl;
    std::cerr << std::endl;
    std::cerr << " Dump info:" << std::endl;
    std::cerr << "   " << program << " -d -i input_file -if input_format" << std::endl;
    std::cerr << std::endl;
    std::cerr << " Help:" << std::endl;
    std::cerr << "   " << program << " -h" << std::endl;
    std::cerr << std::endl;

    std::cerr << "Model formats:" << std::endl;
    std::cerr << " old       => old binary format" << std::endl;
    std::cerr << " new_bin   => new binary format" << std::endl;
    std::cerr << " new_txt   => new text format" << std::endl;
}

bool ParseArgs(int argc, char *argv[])
{
    bool waitI = false, waitO = false;
    bool waitIf = false, waitOf = false;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = std::string(argv[i]);

        if (arg == "-i")
        {
            waitI = true;
            continue;
        }
        if (arg == "-o")
        {
            waitO = true;
            continue;
        }
        if (arg == "-if")
        {
            waitIf = true;
            continue;
        }
        if (arg == "-of")
        {
            waitOf = true;
            continue;
        }

        if (waitI)
        {
            waitI = false;
            ARGS.inputFile = arg;
        }
        else if (waitO)
        {
            waitO = false;
            ARGS.outputFile = arg;
        }
        else if (waitIf)
        {
            waitIf = false;
            ARGS.inputFormat = arg;
        }
        else if (waitOf)
        {
            waitOf = false;
            ARGS.outputFormat = arg;
        }
        else if (arg == "-h")
        {
            PrintUsage(argv[0]);
            ARGS.usage = true;
        }
        else if (arg == "-d")
        {
            ARGS.dumpInfo = true;
        }
        else if (arg == "-m")
        {
            ARGS.mirror = true;
        }
        else
        {
            return false;
        }
    }

    if (waitI || waitO || waitIf || waitOf)
        return false;

    if (ARGS.usage)
        return true;

    if (ARGS.inputFile.empty() || (!ARGS.dumpInfo && ARGS.outputFile.empty() ))
        return false;

    if (ARGS.inputFormat.empty() || (!ARGS.dumpInfo && ARGS.outputFormat.empty() ))
        return false;

    return true;
}

template<typename T>
void PrintStats(const std::map<T, int>& stats, int total)
{
    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        std::cerr << "   " << (*it).first << " : " << (*it).second << " / " << total << std::endl;
    }
}

int main(int argc, char *argv[])
{
    CLogger logger;
    logger.SetLogLevel(LOG_ERROR);

    if (!ParseArgs(argc, argv))
    {
        std::cerr << "Invalid arguments! Run with -h for usage info." << std::endl;
        return 1;
    }

    if (ARGS.usage)
        return 0;

    CInstanceManager iMan;
    Gfx::CModelFile model(&iMan);

    bool ok = true;

    if (ARGS.inputFormat == "old")
    {
        ok = model.ReadModel(ARGS.inputFile);
    }
    else if (ARGS.inputFormat == "new_bin")
    {
        ok = model.ReadBinaryModel(ARGS.inputFile);
    }
    else if (ARGS.inputFormat == "new_txt")
    {
        ok = model.ReadTextModel(ARGS.inputFile);
    }
    else
    {
        std::cerr << "Invalid input format" << std::endl;
        return 1;
    }

    if (!ok)
    {
        std::cerr << "Reading input model failed" << std::endl;
        return 1;
    }

    if (ARGS.dumpInfo)
    {
        const std::vector<Gfx::ModelTriangle>& triangles = model.GetTriangles();

        Math::Vector min( Math::HUGE_NUM,  Math::HUGE_NUM,  Math::HUGE_NUM);
        Math::Vector max(-Math::HUGE_NUM, -Math::HUGE_NUM, -Math::HUGE_NUM);

        std::map<std::string, int> texs1, texs2;
        std::map<int, int> states;
        std::map<float, int> mins, maxs;
        int variableTexs2 = 0;

        for (int i = 0; i < static_cast<int>( triangles.size() ); ++i)
        {
            const Gfx::ModelTriangle& t = triangles[i];

            min.x = Math::Min(t.p1.coord.x, t.p2.coord.x, t.p3.coord.x, min.x);
            min.y = Math::Min(t.p1.coord.y, t.p2.coord.y, t.p3.coord.y, min.y);
            min.z = Math::Min(t.p1.coord.z, t.p2.coord.z, t.p3.coord.z, min.z);

            max.x = Math::Max(t.p1.coord.x, t.p2.coord.x, t.p3.coord.x, max.x);
            max.y = Math::Max(t.p1.coord.y, t.p2.coord.y, t.p3.coord.y, max.y);
            max.z = Math::Max(t.p1.coord.z, t.p2.coord.z, t.p3.coord.z, max.z);

            texs1[t.tex1Name] += 1;
            if (! t.tex2Name.empty())
                texs2[t.tex2Name] += 1;
            if (t.variableTex2)
                variableTexs2 += 1;
            states[t.state] += 1;

            mins[t.min] += 1;
            maxs[t.max] += 1;
        }

        std::cerr << "---- Info ----" << std::endl;
        std::cerr << "Total triangles: " << triangles.size();
        std::cerr << std::endl;
        std::cerr << "Bounding box:" << std::endl;
        std::cerr << " min: [" << min.x << ", " << min.y << ", " << min.z << "]" << std::endl;
        std::cerr << " max: [" << max.x << ", " << max.y << ", " << max.z << "]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Textures:" << std::endl;
        std::cerr << " tex1:" << std::endl;
        PrintStats(texs1, triangles.size());
        std::cerr << " tex2:" << std::endl;
        PrintStats(texs2, triangles.size());
        std::cerr << " variable tex2: " << variableTexs2 << " / " << triangles.size() << std::endl;
        std::cerr << std::endl;
        std::cerr << "States:" << std::endl;
        PrintStats(states, triangles.size());
        std::cerr << std::endl;
        std::cerr << "LOD:" << std::endl;
        std::cerr << " min:" << std::endl;
        PrintStats(mins, triangles.size());
        std::cerr << " max:" << std::endl;
        PrintStats(maxs, triangles.size());

        return 0;
    }

    if (ARGS.mirror)
        model.Mirror();

    if (ARGS.outputFormat == "old")
    {
        ok = model.WriteModel(ARGS.outputFile);
    }
    else if (ARGS.outputFormat == "new_bin")
    {
        ok = model.WriteBinaryModel(ARGS.outputFile);
    }
    else if (ARGS.outputFormat == "new_txt")
    {
        ok = model.WriteTextModel(ARGS.outputFile);
    }
    else
    {
        std::cerr << "Invalid output format" << std::endl;
        return 1;
    }

    if (!ok)
    {
        std::cerr << "Writing output model failed" << std::endl;
        return 1;
    }

    return 0;
}
