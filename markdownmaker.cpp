#include <QFile>
#include <QRegularExpression>
#include <QDate>
#include <QTemporaryFile>
#include "markdownmaker.h"
#include "axq.h"

#include <QDebug>

constexpr char DefaultStyle[] = "##### %1";

static QString decode(const QString& line) {
    QString decoded;
    const QRegularExpression code(R"(@\{((?:x[0-9a-f]+)|(?:\d+))\})", QRegularExpression::CaseInsensitiveOption);
    auto iterator = code.globalMatch(line);
    int pos = 0;
    while(iterator.hasNext()){
        const auto match = iterator.next();
        const auto start = match.capturedStart();
        decoded += line.mid(pos, start - pos);
        bool ok;
        const auto value = match.captured(1);
        decoded += QChar(value[0] == 'x' ? value.mid(1).toInt(&ok, 16) : value.toInt(&ok));
        if(!ok) return "INVALID";
        pos = match.capturedEnd();
    }
    decoded += line.mid(pos);
    return decoded;
}

static QString removeAsterisk(const QString& line) {
    const QRegularExpression ast(R"(\s*\*\s*(.*))");
    const auto m = ast.match(line);
    return m.hasMatch() ? m.captured(1) : line;
}

SourceParser::SourceParser(const QString& name, Styles& styles, QObject* parent) : QObject(parent),
    m_sourceName(name), m_styles(styles) {
    m_scopeStack.push("_root");
    m_scopes.append("_root");
}

bool SourceParser::fail(const QString& s, int line) const {
    auto err = decode(QString("%1, %2 at %3 (ref:%4)").arg(s).arg(m_sourceName).arg(m_line).arg(line));
    err.replace("\n", "");
    err.replace("\r", "");
    err.replace("\\n", "");
    err.replace("\\r", "");
    err.replace("\"", "'");
    err.replace("\\", "");
    qDebug() << err;
    emit const_cast<SourceParser*>(this)->appendLine(err + "<br/>");
    return true;
}

#define S_ASSERT(x, s) if(!x && fail(s, __LINE__)) return false;

SourceParser::~SourceParser() {

}

bool SourceParser::parseLine(const QString& line) {
    ++m_line;
    if(m_state != State::Out) {
        const QRegularExpression example1(R"(```)");
        const QRegularExpression example2(R"(\~\~\~)");
        const QRegularExpression blockCommentEnd(R"(\*/)");
        const QRegularExpression meta(R"(\s*\*\s*@([a-z]+)\s*(.*)(\\n))");
        if(blockCommentEnd.match(line).hasMatch()) {
            m_state = State::Out;
        } else {
            const auto bm = meta.match(line);
            if(bm.hasMatch()) {
                const auto command = bm.captured(1);
                const auto value = decode(bm.captured(2));
                if(command == "scope" || command == "class" || command == "namespace" || command == "struct") {
                    m_scopes.append(value);
                    m_scopeStack.push(value);
                    m_content[m_scopeStack.top()].append({Cmd::Add, "", "---\\n"});
                }
                if(command == "class" || command == "namespace" || command == "typedef") {
                    m_links.append({command, value});
                    m_content[m_scopeStack.top()].append({Cmd::Header, command, value});
                }

                else if(command == "toc") {
                    m_content[m_scopeStack.top()].append({Cmd::Toc, "", ""});
                } else if(command == "date") {
                    m_content[m_scopeStack.top()].append({Cmd::Header, command, QDate::currentDate().toString()});
                } else if(command == "scopeend") {
                    m_content[m_scopeStack.top()].append({Cmd::Add, "", "---\\n"});
                    m_scopeStack.pop();
                } else if(command == "style") {
                    auto sep = value.indexOf(' ');
                    if(sep > 0) {
                        m_styles.setStyle(value.left(sep), value.mid(sep + 1));
                    } else {
                        qWarning() << "Invalid style" << value;
                    }
                } else if(command == "function") {
                    S_ASSERT(!m_briefName, QString("Only one brief or function allowed: %1 ").arg(line));
                    m_content[m_scopeStack.top()].append({Cmd::Header, command, value});
                    m_briefName = & m_content[m_scopeStack.top()].last();
                } else if(command == "raw") {
                    m_content[m_scopeStack.top()].append({Cmd::Add, "", value});
                } else if(command == "eol") {
                    m_content[m_scopeStack.top()].append({Cmd::Add, "", "\\n"});
                } else if(command == "ignore") {
                   //ignore
                } else {
                    m_content[m_scopeStack.top()].append({Cmd::Header, command, value});
                }
            } else if(m_state != State::Example2 && example1.match(line).hasMatch()) {
                if(m_state == State::In) {
                    m_state = State::Example1;
                } else {
                    m_state = State::In;
                }
                m_content[m_scopeStack.top()].append({Cmd::Add, "", "```\\n"});  
            } else if(m_state != State::Example1 && example2.match(line).hasMatch()) {
                if(m_state == State::In) {
                    m_state = State::Example2;
                } else {
                    m_state = State::In;
                }
                m_content[m_scopeStack.top()].append({Cmd::Add, "", "~~~\\n"});
            } else if(m_state == State::In) {
                const auto ref = decode(removeAsterisk(line));
                m_content[m_scopeStack.top()].append({Cmd::Add, "", ref.toHtmlEscaped()});
            } else if(m_state == State::Example1 || m_state == State::Example2) {
                auto ref = decode(removeAsterisk(line));
                ref.replace("\\n", "");
                ref.replace('\\', "\\\\");
                ref.replace('"', "\\\"");
                m_content[m_scopeStack.top()].append({Cmd::Add, "", ref + "  \\n"});
            }
        }
    } else {
        if(m_briefName) {
            auto functionName = line;
            functionName.replace(QRegularExpression(R"(<[^>])"), "<>");
            functionName.replace(QRegularExpression(R"(\([^\)])"), "()");
            const QRegularExpression function(R"((^\s*|\S+\s)+(?<name>[a-zA-Z_][a-zA-Z0-9_]*)\s*\(|<)");
            const auto m = function.match(functionName);
            if(m.hasMatch() && m.captured("name") == std::get<2>(*m_briefName)) {
                const QRegularExpression functionTail(R"(^(.*\)($|\s?[a-zA-Z_]+)?))");
                const auto fm = functionTail.match(line);
                S_ASSERT(fm.hasMatch(), QString("Cannot understand as a function %1").arg(line));
                auto value = fm.captured(0).trimmed();
                value.remove(QRegularExpression(R"(^\s*\w+_EXPORT)"));
                *m_briefName = {Cmd::Header, std::get<1>(*m_briefName), value};
                m_links.append({std::get<1>(*m_briefName), value});
                m_briefName = nullptr;
            }
        }
        const QRegularExpression mdCommentStart(R"(/\*\*)");
        if(mdCommentStart.match(line).hasMatch()) {
            m_state = State::In;
            S_ASSERT(!m_briefName, QString("function not found \\\"%1\\\"").arg(std::get<2>(*m_briefName)));
        }
    }
    return true;
}


QString SourceParser::makeLink(const Link& link) const {
    return "#" + std::get<1>(link).toLower().replace(QRegularExpression("[<>]"), "").replace(QRegularExpression(R"(\W+)"), "-");
}

void SourceParser::complete() {
    Axq::from(m_scopes).
    each<QString>([this](const QString & scope) {
        for(const auto& line :  m_content[scope]) { //we cannot be async here as this has append in seq
            const auto cmd = std::get<0>(line);
            switch(cmd) {
            case Cmd::Add:
                emit appendLine(std::get<2>(line));
                break;
            case Cmd::Toc:
                for(const auto& link : m_links) {
                    const auto uri = makeLink(link);
                    emit appendLine("* [" + std::get<1>(link) + " ](" +  uri + ")" + "\\n");
                }
                break;
            case Cmd::Header:
                emit appendLine(QString(m_styles.style(std::get<1>(line)) + " \\n").arg(std::get<2>(line)));
                break;
            }
        }
    })
    .onCompleted([this]() {
        emit completed();
    });
}


MarkdownMaker::MarkdownMaker(QObject* parent) : QObject(parent) {
    setStyle("namespace", "### %1");
    setStyle("class", "#### %1");
    setStyle("param", "###### *Param:* %1");
    setStyle("return", "###### *Return:* %1");
    setStyle("templateparam", "###### *Template arg:* %1");
    setStyle("brief", "###### %1");
    setStyle("date", "###### %1");
    QObject::connect(this, &MarkdownMaker::contentChanged, this, [this]() {
        ++m_completed;
        if(m_completed == 0) {
            emit appendLine("###### Generated by MarkupMaker, (c) Markus Mertama 2018 \\n");
            emit contentChanged();
            emit allMade();
        }
    }, Qt::QueuedConnection);
}

void MarkdownMaker::addMarkupFile(const QString& mdFile) {
    QFile* f = new QFile(mdFile);
    --m_completed;
    Axq::read(f)
    .map<QString, QString>([](QString s) {
        s.replace('\n', "\\n");
        return s;
    })
    .each<QString>([this, mdFile](const QString & s) {
        m_content[mdFile] += s.toHtmlEscaped();
    })
    .onError<QString>([this, mdFile](const QString&, int) {
        m_content[""] += "cannot load markup file:" + mdFile;
        emit contentChanged();
    })
    .onCompleted([this]() {
        emit contentChanged();
    });
    if(f->isOpen()) {
        m_files.append(mdFile);
    }
}

void MarkdownMaker::addSourceFile(const QString& sourceFile) {
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

QString MarkdownMaker::content() const {
    QString data;
    for(const auto& names : m_files) {
        data += m_content[names];
    }
    return data;
}

void MarkdownMaker::setStyle(const QString& name, const QString& style) {
    m_styles[name] = style;
}

QString MarkdownMaker::style(const QString& name) const {
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

void MarkdownMaker::setOutput(const QString& out) {
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

void MarkdownMaker::setSourceFiles(const QList<QUrl>& files) {
    m_content.clear();
    for(const auto& f : files){
        addSourceFile(f.toLocalFile());
    }
}

void MarkdownMaker::setOutput(const QUrl& file) {
    if(!m_hasOutput)
        doCopy(file.toLocalFile());
    else {
            qCritical() << "output already set";
        }
}



