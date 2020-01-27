
#include "markdownmaker.h"

std::string absoluteFilePath(const std::string& name);

int main(int argc, char* argv[]) {
   MarkdownMaker mm;

   std::vector<std::string> files;
   bool isQuiet = false;

    for(auto i = 1 ; i < argc; i++) {
        std::string arg(argv[i]);
        if(arg.front() == '-') {
            auto p = arg.substr(1);
            if(p == "o" && i < argc - 1) {
                mm.setOutput(argv[i + 1]);
            }
            if(p == "q") {
                isQuiet = true;
            }
        } else {
            files.push_back(arg);
        }
    }



    /*if(!mm.hasOutput()){
        mm.setOutput("");
    }*/

    for(const auto& f : files) {
        if(toLower(f.substr(f.find_last_of(".") + 1)) == "md") {
            mm.addMarkupFile(absoluteFilePath(f));
        } else {
            mm.addSourceFile(absoluteFilePath(f));
        }
    }
}
