#ifndef MARKUPMAKER_H
#define MARKUPMAKER_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <stack>

/**
  * ![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&v=4)
  * # MarkdownMaker
  *
  * MarkdownMaker is a simple application to make markdown documentation. Markdown is
  * is a simple markup language to write formatted documents, widely used e.g. in GitHub.
  * MarkdownMaker is not a replacement of Doxygen or other document generators; MarkdownMaker
  * purpose and approach is different: it generates markdown only annotated comments and
  * ignores everything else. Therefore where Doxygen is better for generating comprehensive
  * documentation of source code, MarkdownMaker generates only explicit documentation and
  * is therefore it is suitable for interface (API) and introductionary (README) documentation.
  * Also, unlike those other generators, MarkdownMaker just generates markdown.
  *
  * The generated markdown is optinally rendered, but please note that is only referential and may
  * differ e.g. from Github's interpretation of the document.
  *
  * #### Command line
  * @raw `mdmaker <-q> <-o OUTFILE> <INFILES>`
  * @eol
  * * **mdmaker** Since the executable may have been wrapped into bundle, the actual callable name may vary.
  * * -q , Quiet, no UI, suitable for toolchains.
  * * -o OUTPUT, Write output to given file, if not given, Save as dialog is shown upon exit. If OUTPUT
  * is *null* nothing is written and no dialog is shown.
  * * INFILES, One or more files that are scanned for markdown annotations. Multiple files are joined
  * as a single output in input order. If input is a markdown file (*.md), that is added as-is.
  * If no files given a file open dialog is shown.

  *
  *  #### Shortcuts
  *  `Cmd/Ctrl+Q` enforces MarkdownMaker to quit without asking Save as dialog.
  *
  * ### Annotations
  * #### Markdown field
  * @raw Markdown section starts with __/&lowast;&lowast;__ ( C comment with an extra asterisk).
  * @raw Section ends to __&lowast;/__ (C comment ends). If section line starts with a __*__, that is omitted.
  *
  * The section can include markdown language. There it is possible to use `code`,
  * *italic* and other [items](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet)
  *
  * #### Tokens
  * @raw + __&#x40;toc__, generates table of contents. Items are pointing to &#x40;namespace, &#x40;class and &#x40;function tokens in this document.
  * @eol
  * @raw + __&#x40;scope__, starts a generic scope. Scopes are used to divide markdown content.
  * @eol
  * @raw + __&#x40;scopeend__, ends any scope (i.e. also namespace and class scope)
  * @eol
  * @raw + __&#x40;namespace NAME__, generates a namespace header and starts a scope.
  * @eol
  * @raw + __&#x40;class NAME__, generates a class header and starts a scope.
  * @eol
  * @raw + __&#x40;function NAME__, just name, no return value or parameters, , generates a function header. The actual function name string is generated from the function declaration that is expected to found immediately after this markdown section.
  * @eol
  * @raw + __&#x40;templateparam NAME description__, generates a template parameter header.
  * @eol
  * @raw + __&#x40;param NAME description__, generates a parameter header.
  * @eol
  * @raw + __&#x40;return description__, generates a description header.
  * @eol
  * @raw + __&#x40;brief__, adds a general one-liner
  * @eol
  * @raw + __&#x40;date__, injects a current date.
  * @eol
  * @raw + __&#x40;style__ TOKEN STYLE-DESCRIPTION, let redefine styles. The STYLE-DESCRIPTION is generated markdown syntax, where %1 represents token's content.
  * @eol
  * @raw + __&#x40;raw__ RAW-TEXT, Normally the inserted text is HTML escaped, but that can be bypassed using raw token and RAW-TEXT is HTML interpreted as-is
  * @eol
  * @raw + __&#x40;eol__, after &#x40;raw can join lines and then &#x40;eol is used to insert an explicit newline and end the &#x40;raw content.
  * @eol
  * @raw + __&#x40;ignore__, The line is ignored from output.
  * @eol
  *
  * #### Coded value
  * * @raw + __&#x40;{`unicode-code`}__, MarkdownMaker replaces a given `unicode-code` with its Unicode character (Hexadecimal values start with 'x'). That let to use characters not otherwise allowed in C/C++ source files.
  * @eol
  *
  *  #### Examples
  * Using Style
```
 *  ###### *Axq is published under GNU LESSER GENERAL PUBLIC LICENSE, Version 3*
 *  @style date ###### *Copyright Markus Mertama 2018*, generated at %1
 *  @date
 *  _____
```
  * Using Raw
```
  @raw + __&#x40;return decription__, generates a description header.
  @eol
```
  *
  * Using to declare a function
  *
  ~~~
template <typename ...Args>
/@{x2a}@{x2a}
 @{x2a} @function tie
 @{x2a} Apply list values to arguments
 @{x2a} @param ParamList object to read from
 @{x2a} @param list of out parameters to read values to
 @{x2a}
 @{x2a}
 @{x2a} Example:
 ```
 stream.each<Axq::ParamList>([this](const Axq::ParamList& pair){
    QString key; int count;
    Axq::tie(pair, key, count);
    print("\"", key, "\" exists ", count, " times\n");
    ...
 ```
 @{x2a}
 @{x2a}/
void tie(const ParamList& list, Args& ...args) {
    _getValue<0, Args...>(list, args...);
}
 ~~~
  * @eol
  *  See [AxqLib documentation](https://github.com/mmertama/AxqLib/blob/master/Axq.md)
  *  ...and [its source](https://github.com/mmertama/AxqLib/blob/master/axq.h)
  *  ...and [this source](https://github.com/mmertama/MarkdownMaker/blob/master/markdownmaker.h)
  * , Axq is also used for implementation of this utility.
  */

class Styles {
public:
    virtual void setStyle(const std::string& name, const std::string& style) = 0;
    virtual std::string style(const std::string& name) const = 0;
    virtual ~Styles() = default;
};


class ContentManager : public Styles {
  public:
    std::function<void (const std::string& line)> appendLine = nullptr;
};

class SourceParser  {
    enum class State {Out, In, Example1, Example2};
    enum class Cmd {Add, Toc, Header};
    using Link = std::tuple<std::string, std::string>;
public:
    using Content = std::tuple<Cmd, std::string, std::string>;
public:
    SourceParser(const std::string& sourceName, ContentManager& styles);
    ~SourceParser();
    bool parseLine(const std::string& line);
    void complete();
    void appendLine(const std::string& str) {m_contentManager.appendLine(str);}
private:
    std::string makeLink(const Link& lnk) const;
    bool fail(const std::string& message, int line) const;
private:
    const std::string m_sourceName;
    ContentManager& m_contentManager;
    State m_state = State::Out;
    std::map<std::string, std::vector<Content>> m_content;
    std::vector<Link> m_links;
    std::stack<std::string> m_scopeStack;
    std::vector<std::string> m_scopes;
    Content* m_briefName = nullptr;
    int m_line = 0;
};


class MarkdownMaker : public ContentManager {
public:
    explicit MarkdownMaker();
    void addMarkupFile(const std::string& mdFile);
    void addSourceFile(const std::string& sourceFile);
    void setOutput(const std::string& out);
    void addFooter();
    bool hasOutput() const;
    bool hasInput() const;
 //   void setSourceFiles(const std::vector<std::string>& files);
 //   void setOutput(const std::string& file);

//    void appendLine(const std::string& line);
//    void allMade();
    void contentChanged() {std::for_each(contentChangedArray.begin(), contentChangedArray.end(), [](const auto& f){f();});}
    void showFileOpen();
    void doCopy(const std::string& target);
public:
    std::string content() const;
    void setStyle(const std::string& name, const std::string& style);
    std::string style(const std::string& name) const;
private:
    std::vector<std::string> m_files;
    std::unordered_map<std::string, std::string> m_content;
    std::unordered_map<std::string, std::string> m_styles;
    int m_completed = 0;
    bool m_hasOutput = false;
    std::vector<std::function<void ()>> contentChangedArray;
};

inline std::string toLower(const std::string& s) {
    std::string out;
    std::transform(s.begin(), s.end(), std::back_inserter(out), [](auto c){return std::tolower(c);});
    return out;
}

#endif // MARKUPMAKER_H
