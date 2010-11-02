
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QSet>
#include <QTextStream>
#include <QVector>


class FileClass
{
public:
    FileClass() {}

    //! suffixes is a comma separated string of extensions.
    FileClass(const QByteArray &suffixes, const QString &varName)
        : m_suffixes(',' + suffixes + ','), m_varName(varName)
    {}

    static QByteArray prepareSuffix(const QByteArray &suffix)
    {
        return ',' + suffix + ',';
    }

    bool canHandle(const QByteArray &preparedSuffix) const
    {
        return m_suffixes.contains(preparedSuffix);
    }

    void addFile(const QFileInfo &fi)
    {
        m_files.insert(fi.filePath());
    }

    bool handleFile(const QFileInfo &fi, const QByteArray &preparedSuffix)
    {
        if (!canHandle(preparedSuffix))
            return false;
        addFile(fi);
        return true;
    }

    void writeProBlock(QTextStream &ts) const
    {
        ts << '\n' << m_varName << " *=";
        foreach (QString s, m_files)
            ts << " \\\n    " << s;
        ts << "\n";
    }

private:
    QByteArray m_suffixes;
    QString m_varName;
    QSet<QString> m_files;
};

class Runner
{
public:
    Runner() {}
    void run();
    void addPath(const QDir &dir);
    void addFileClass(const FileClass &fc) { m_fileClasses.append(fc); }
    void writeProFile(const QString &fileName);

private:
    void handleDir(const QDir &dir);

    QList<QDir> m_dirs;
    QVector<FileClass> m_fileClasses;
};

void Runner::addPath(const QDir &dir)
{
    m_dirs.append(dir);
}

void Runner::run()
{
    for (int i = 0; i != m_dirs.size(); ++i)
        handleDir(m_dirs.at(i));
}

void Runner::handleDir(const QDir &dir)
{
    QDirIterator it(dir.path());
    while (it.hasNext()) {
        it.next();
        const QFileInfo &fi = it.fileInfo();
        if (fi.isDir()) {
            if (fi.fileName() != ".." && fi.fileName() != ".")
                addPath(fi.filePath());
        } else {
            const QByteArray ext = FileClass::prepareSuffix(fi.suffix().toUtf8());
            for (int i = m_fileClasses.size(); --i >= 0; ) {
                if (m_fileClasses[i].handleFile(fi, ext))
                    break;
            }
        }
    }
}

void Runner::writeProFile(const QString &fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadWrite);
    QTextStream ts(&file);
    ts << "######################################################################\n";
    ts << "# Automatically generated by qtpromaker\n";
    ts << "######################################################################\n\n";
    ts << "TEMPLATE = app\n";
    ts << "TARGET = " << QFileInfo(fileName).baseName() << "\n";
    foreach (const FileClass &fc, m_fileClasses)
        fc.writeProBlock(ts);
    ts << "\nPATHS *=";
    foreach (const QDir &dir, m_dirs)
        ts << " \\\n    " << dir.path();
    ts << "\n\nDEPENDPATH *= $$PATHS\n";
    ts << "\nINCLUDEPATH *= $$PATHS\n";
    ts.flush();
    file.close();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    QDir dir = QDir::currentPath();
    QString outFile = dir.dirName() + ".pro";
    QStringList paths;

    for (int i = 1, n = args.size(); i < n; ++i) {
        const QString arg = args.at(i);
        if (arg == "-h" || arg == "--help" || arg == "-help") {
            qWarning() << "Usage: " << qPrintable(args.at(0))
                << " [-o out.pro] [dir...]";
            return 1;
        } else if (arg == "-o" && i < n - 1) {
            outFile = args.at(++i);
        } else {
            paths.append(args.at(i));
        }
    }

    if (paths.isEmpty())
        paths.append(".");

    Runner r;
    // FIXME: Make file classes configurable on the command line.
    r.addFileClass(FileClass("cpp,c,C,cxx,c++", "SOURCES"));
    r.addFileClass(FileClass("hpp,h,H,hxx,h++", "HEADERS"));
    r.addFileClass(FileClass("ts", "TRANSLATIONS"));
    r.addFileClass(FileClass("ui", "FORMS"));
    foreach (const QString &path, paths)
        r.addPath(QDir(path));

    r.run();

    r.writeProFile(outFile);

    return 0;
}