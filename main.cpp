
#include "markdownmaker.h"
#include <iostream>

std::string absoluteFilePath(const std::string& name);

int main(int argc, char* argv[]) {
   MarkdownMaker mm;

   std::vector<std::string> files;

    for(auto i = 1 ; i < argc; i++) {
        std::string arg(argv[i]);
        if(arg.front() == '-') {
            auto p = arg.substr(1);
            if(p == "o" && i < argc - 1) {
                mm.setOutput(argv[i + 1]);
            }

        } else {
            files.push_back(arg);
        }
    }

    if(files.size() == 0) {
        std::cerr << "<-o outfile> infiles" << std::endl;
        return -1;
    }



    if(!mm.hasOutput()){
        mm.setOutput("");
    }

    for(const auto& f : files) {
        const auto fname = absoluteFilePath(f);
        if(fname.empty()) {
            std::cerr << "Cannot open:" << f << std::endl;
            return -1;
        }
        if(toLower(f.substr(f.find_last_of(".") + 1)) == "md") {   
            mm.addMarkupFile(f);
        } else {
            mm.addSourceFile(f);
        }
    }

  //  std::cout << mm.content() << std::endl;
    return 0;
}
