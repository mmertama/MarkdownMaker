#ifndef MARKUPMAKER_H
#define MARKUPMAKER_H

#include <QObject>
#include <QHash>
#include <QStack>
#include <QMap>
#include <QUrl>

/**
  * ![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&v=4)
  * # MarkdownMaker
  *
  * MarkdownMaker is a simple application to make markdown documentation. Markdown is
  * is a simple markup language to write formatted documents, widely used e.g. in GitHub.
  * MarkdownMaker is not a replacement of Doxygen or other document generators; MarkdownMaker
  * purpose and approach is different: it generates markdown only annotated comments and
  * ignores everything else. Therefore where Doxygen is better for generating comprehensive
  * documentation of source code, MarkdownMaker adds generates only explicit documentation and
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
    virtual void setStyle(const QString& name, const QString& style) = 0;
    virtual QString style(const QString& name) const = 0;
    virtual ~Styles() = default;
};

class SourceParser : public QObject {
    Q_OBJECT
    enum class State {Out, In, Example1, Example2};
    enum class Cmd {Add, Toc, Header};
    using Link = std::tuple<QString, QString>;
public:
    using Content = std::tuple<Cmd, QString, QString>;
public:
    SourceParser(const QString& sourceName, Styles& styles, QObject* parent = nullptr);
    ~SourceParser();
    bool parseLine(const QString& line);
    void complete();
signals:
    void appendLine(const QString& str);
    void completed();
private:
    QString makeLink(const Link& lnk) const;
    bool fail(const QString& message, int line) const;
private:
    const QString m_sourceName;
    Styles& m_styles;
    State m_state = State::Out;
    QMap<QString, QList<Content>> m_content;
    QList<Link> m_links;
    QStack<QString> m_scopeStack;
    QList<QString> m_scopes;
    Content* m_briefName = nullptr;
    int m_line = 0;
};

Q_DECLARE_METATYPE(SourceParser::Content);

class MarkdownMaker : public QObject, public Styles {
    Q_OBJECT
    Q_PROPERTY(QString content READ content NOTIFY contentChanged)
public:
    explicit MarkdownMaker(QObject* parent = nullptr);
    void addMarkupFile(const QString& mdFile);
    void addSourceFile(const QString& sourceFile);
    void setOutput(const QString& out);
    void addFooter();
    Q_INVOKABLE bool hasOutput() const;
    Q_INVOKABLE bool hasInput() const;
    Q_INVOKABLE void setSourceFiles(const QList<QUrl>& files);
    Q_INVOKABLE void setOutput(const QUrl& file);
signals:
    void appendLine(const QString& line);
    void contentChanged();
    void allMade();
    void showFileOpen();
    void doCopy(const QString& target);
public:
    QString content() const;
    void setStyle(const QString& name, const QString& style);
    QString style(const QString& name) const;
private:
    QStringList m_files;
    QHash<QString, QString> m_content;
    QHash<QString, QString> m_styles;
    int m_completed = 0;
    bool m_hasOutput = false;
};

#endif // MARKUPMAKER_H
