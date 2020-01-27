#include "markdownmaker.h"
#include <regex>
#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>

constexpr char DefaultStyle[] = "##### %1";

static std::string dateNow() {
     const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
     return std::ctime(&now);
}

static std::string replace(const std::string& where, const std::string& what, const std::string& how) {
    std::regex re(what);
    return std::regex_replace(where, re, how);
}

static std::string htmlEscaped(const std::string& str) {
    const std::unordered_map<char, std::string> pairs {
        {'"', "&quot;"},
        {'&', "&amp;"},
        {'\'', "&#39;"},
        {'<', "&lt;"},
        {'>', "&gt;"}};
    std::string out = str;
    for(const auto& [k, v] : pairs) {
        int pos = 0;
        std::string replaced;
        for(;;) {
            const auto index = out.find_first_of(k, pos);
            if(index == std::string::npos)
                break;
            replaced += out.substr(pos, index - pos);
            replaced += v;
            pos = index + v.length();
        }
        replaced += out.substr(pos);
        out = replaced;
    }
    return out;
}

static std::string trim(const std::string &cs) {
    auto s  = cs;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto ch) {
        return !std::isspace(ch);}));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](auto ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}


/*This is maybe a bit clumsy as a quick port from the logic used with QRegularExpression*/
static std::string decode(const std::string& line) {
    std::string decoded;
    const std::regex code(R"(@\{((?:x[0-9a-f]+)|(?:\d+))\})", std::regex::icase);
    std::smatch match;
    int pos = 0;
    while(std::regex_search(line, match, code)) {
        const auto start = match.position();
        decoded += line.substr(pos, start - pos);
        const auto value = match[1].str();
        try {
        decoded += char(value.at(0) == 'x' ? std::stoi(value.substr(1), 0, 16) : std::stoi(value.substr(1)));
        } catch(std::invalid_argument) {
            return "INVALID";
        }
        pos = match.position() + match.length();
    }
    decoded += line.substr(pos);
    return decoded;
}

static std::string removeAsterisk(const std::string& line) {
    const std::regex ast(R"(\s*\*\s*(.*))");
    std::smatch match;
    return std::regex_match(line, match, ast) ?
                match[1].str() : line;
}

SourceParser::SourceParser(const std::string& name, Styles& styles) :
    m_sourceName(name), m_styles(styles) {
    m_scopeStack.push("_root");
    m_scopes.push_back("_root");
}

bool SourceParser::fail(const std::string& s, int line) const {
    auto err = decode(s + ", " + m_sourceName + " at " +
                      std::to_string(m_line) + " (ref:(" +
                      std::to_string(line) + ")");

    replace(err, "\n", "");
    replace(err, "\r", "");
    replace(err, "\\n", "");
    replace(err, "\\r", "");
    replace(err, "\"", "'");
    replace(err, "\\", "");
    std::cerr << err;
    const_cast<SourceParser*>(this)->appendLine(err + "<br/>");
    return true;
}


#define S_ASSERT(x, s) if(!x && fail(s, __LINE__)) return false;

SourceParser::~SourceParser() {
}

bool SourceParser::parseLine(const std::string& line) {
    ++m_line;
    if(m_state != State::Out) {
        const std::regex example1(R"(```)");
        const std::regex example2(R"(\~\~\~)");
        const std::regex blockCommentEnd(R"(\*/)");
        const std::regex meta(R"(\s*\*\s*@([a-z]+)\s*(.*)(\\n))");
        std::smatch match;
        if(std::regex_match(line, match, blockCommentEnd)) {
            m_state = State::Out;
        } else {
            std::smatch bm;
            if(std::regex_match(line, bm, meta)) {
                const auto command = bm[1].str();
                const auto value = decode(bm[2].str());
                if(command == "scope" || command == "class" || command == "namespace" || command == "struct") {
                    m_scopes.push_back(value);
                    m_scopeStack.push(value);
                    m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Add, "", "---\\n"));
                }
                if(command == "class" || command == "namespace" || command == "typedef") {
                    m_links.push_back(std::make_tuple(command, value));
                    m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Header, command, value));
                }

                else if(command == "toc") {
                    m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Toc, "", ""));
                } else if(command == "date") {
                    m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Header, command, dateNow()));
                } else if(command == "scopeend") {
                    m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Add, "", "---\\n"));
                    m_scopeStack.pop();
                } else if(command == "style") {
                    auto sep = value.find_first_of(' ');
                    if(sep > 0) {
                        m_styles.setStyle(value.substr(0, sep), value.substr(sep + 1));
                    } else {
                        std::cerr << "Invalid style" << value;
                    }
                } else if(command == "function") {
                    S_ASSERT(!m_briefName, "Only one brief or function allowed:" + line);
                    m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Header, command, value));
                    m_briefName = & m_content[m_scopeStack.top()].back();
                } else if(command == "raw") {
                    m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Add, "", value));
                } else if(command == "eol") {
                    m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Add, "", "\\n"));
                } else if(command == "ignore") {
                   //ignore
                } else {
                    m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Header, command, value));
                }
            } else if(m_state != State::Example2 && std::regex_match(line, match, example1)) {
                if(m_state == State::In) {
                    m_state = State::Example1;
                } else {
                    m_state = State::In;
                }
                m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Add, "", "```\\n"));
            } else if(m_state != State::Example1 && std::regex_match(line, match, example2)) {
                if(m_state == State::In) {
                    m_state = State::Example2;
                } else {
                    m_state = State::In;
                }
                m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Add, "", "~~~\\n"));
            } else if(m_state == State::In) {
                const auto ref = decode(removeAsterisk(line));
                m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Add, "", htmlEscaped(ref)));
            } else if(m_state == State::Example1 || m_state == State::Example2) {
                auto ref = decode(removeAsterisk(line));
                replace(ref, "\\n", "");
                replace(ref, "\\", "\\\\");
                replace(ref, "\"", "\\\"");
                m_content[m_scopeStack.top()].push_back(std::make_tuple(Cmd::Add, "", ref + "  \\n"));
            }
        }
    } else {
        if(m_briefName) {
            auto functionName = line;
            replace(functionName, R"(<[^>])", "<>");
            replace(functionName, R"(\([^\)])", "()");
            const std::regex function(R"((^\s*|\S+\s)+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(|<)");
            std::smatch match;
            if(std::regex_match(functionName, match, function) && match[2] == std::get<2>(*m_briefName)) {
                const std::regex functionTail(R"(^(.*\)($|\s?[a-zA-Z_]+)?))");
                if(!std::regex_match(line, match, functionTail)) {
                    S_ASSERT(false, "Cannot understand as a function:" +line);
                }
                auto value = trim(match[0]);
                value = replace(value, R"(^\s*\w+_EXPORT)", "");
                *m_briefName = std::make_tuple(Cmd::Header, std::get<1>(*m_briefName), value);
                m_links.push_back(std::make_tuple(std::get<1>(*m_briefName), value));
                m_briefName = nullptr;
            }
        }
        const std::regex mdCommentStart(R"(/\*\*)");
        std::smatch match;
        if(std::regex_match(line, match, mdCommentStart)) {
            m_state = State::In;
            S_ASSERT(!m_briefName, "function not found \\\"" + std::get<2>(*m_briefName) + "\\\"");
        }
    }
    return true;
}


std::string SourceParser::makeLink(const Link& link) const {
    return "#" + replace(replace(toLower(std::get<1>(link)), "[<>]", ""), R"(\W+)", "-");
}

void SourceParser::complete() {
    for(const auto scope : m_scopes) {
        for(const auto& line :  m_content[scope]) { //we cannot be async here as this has append in seq
            const auto cmd = std::get<0>(line);
            switch(cmd) {
            case Cmd::Add:
                appendLine(std::get<2>(line));
                break;
            case Cmd::Toc:
                for(const auto& link : m_links) {
                    const auto uri = makeLink(link);
                    appendLine("* [" + std::get<1>(link) + " ](" +  uri + ")" + "\\n");
                }
                break;
            case Cmd::Header:
                appendLine(replace(m_styles.style(std::get<1>(line)) + " \\n", "%1", std::get<2>(line)));
                break;
            }
        }
    }
    completed();
}

MarkdownMaker::MarkdownMaker() {
    setStyle("namespace", "### %1");
    setStyle("class", "#### %1");
    setStyle("param", "###### *Param:* %1");
    setStyle("return", "###### *Return:* %1");
    setStyle("templateparam", "###### *Template arg:* %1");
    setStyle("brief", "###### %1");
    setStyle("date", "###### %1");
}

void MarkdownMaker::contentChanged() {
    ++m_completed;
    if(m_completed == 0) {
        appendLine("###### Generated by MarkdownMaker, (c) Markus Mertama 2018 \\n");
        contentChanged();
        allMade();
    }
}

void MarkdownMaker::addMarkupFile(const std::string& mdFile) {
    std::ifstream f(mdFile);
    std::string line;
    --m_completed;
    if(f.is_open()) {
        m_files.push_back(mdFile);
    } else {
        m_content[""] += "cannot load markup file:" + mdFile;
        contentChanged();
    }
    while (std::getline(f, line)) {
        line  = replace(line, "\n", "\\n");
        m_content[mdFile] += htmlEscaped(line);
    }
    contentChanged();
}

void MarkdownMaker::addSourceFile(const std::string& sourceFile) {
    auto file = new QFile(sourceFile);
    --m_completed;
    auto parser = new SourceParser(sourceFile, *this);
    QObject::connect(parser, &SourceParser::appendLine, this, &MarkdownMaker::appendLine);
    QObject::connect(this, &MarkdownMaker::appendLine, [this, sourceFile](const QString & append) {
        m_content[sourceFile] += append;
    });
    Axq::read(file)
    .own(parser)
    .onError<QString>([this, sourceFile](const QString&, int) {
        m_content[""] += "cannot load source file:" + sourceFile;
    })
    .meta<QString, Axq::Stream::This>([parser](QString line, Axq::Stream s) {
        line.replace("\r\n", "\n"); //DOS line endings
        if(!parser->parseLine(line.replace('\n', "\\n"))) {
            s.error("Parse error", -1, true);
        }
        return line;
    })
    .onCompleted([this, parser](Axq::Stream s) {
        Axq::from(parser)
        .take(s)
        .wait(parser, &SourceParser::completed)
        .each<SourceParser*>([](SourceParser * p) {
            p->complete();
        })
        .onCompleted([this]() {
            emit contentChanged();
        });
    }).onError<QString>([this](const QString, int) {
        emit contentChanged();
    });
    if(file->isOpen()) {
        m_files.append(sourceFile);
    } else {
        qCritical() << "Cannot open" << sourceFile;
    }
}

std::string MarkdownMaker::content() const {
    std::string data;
    for(const auto& names : m_files) {
        data += m_content[names];
    }
    return data;
}

void MarkdownMaker::setStyle(const std::string& name, const std::string& style) {
    m_styles[name] = style;
}

std::string MarkdownMaker::style(const std::string& name) const {
    if(!m_styles.contains(name)) {
        return DefaultStyle;
    } else {
        return m_styles[name];
    }
}

bool MarkdownMaker::hasOutput() const {
    return m_hasOutput;
}

bool MarkdownMaker::hasInput() const {
    return !m_content.isEmpty();
}

void MarkdownMaker::setOutput(const std::string& out) {
    if(out == "null") {
        m_hasOutput = true; //Not really
        return;
    }
    auto f = !out.isEmpty() ? new QFile(out, this) : new QTemporaryFile(this);
    if(f->open(QIODevice::WriteOnly)) {
        QObject::connect(this, &MarkdownMaker::appendLine, [f](QString append) {
            append.replace("\\n", "\n");
            append.replace(QRegularExpression(R"(\\(.))"), "\\1");
            f->write(append.toUtf8());
        });
        QObject::connect(this, &MarkdownMaker::contentChanged, [f]() {
            f->flush();
        });
        if(!out.isEmpty())
            m_hasOutput = true;
        else {
            QObject::connect(this, &MarkdownMaker::doCopy, [f](const QString& target){
                f->flush();
                QFile::remove(target);
                QFile::copy(f->fileName(), target);
            });
        }
    } else {
        m_content[""] += "cannot open output:" + out;
    }
}

void MarkdownMaker::setSourceFiles(const std::vector<std::string>& files) {
    m_content.clear();
    for(const auto& f : files){
        addSourceFile(f.toLocalFile());
    }
}

void MarkdownMaker::setOutput(const std::string& file) {
    if(!m_hasOutput) {
        doCopy(file);
    } else {
            std::cerr << "output already set";
        }
}



