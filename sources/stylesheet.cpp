#include "stylesheet.h"
#include "icctransform.h"
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QApplication>
#include <QMetaEnum>

QScopedPointer<Stylesheet, Stylesheet::Deleter> Stylesheet::pi;

class StylesheetPrivate : public QObject {
    Q_OBJECT
public:
    StylesheetPrivate();
    ~StylesheetPrivate();

    QString path;
    QString compiled;
    QHash<QString, QColor> palette;
};

StylesheetPrivate::StylesheetPrivate() {}
StylesheetPrivate::~StylesheetPrivate() {}
#include "stylesheet.moc"

Stylesheet::Stylesheet()
    : p(new StylesheetPrivate())
{
    ICCTransform* transform = ICCTransform::instance();
    auto map = [&](ColorRole role, QColor c) {
        QColor mapped = transform->map(c.rgb());
        setColor(role, mapped);
    };
    map(Base, QColor::fromHsl(220, 76, 6));
    map(BaseAlt, QColor::fromHsl(220, 30, 12));
    map(Accent, QColor::fromHsl(220, 6, 20));
    map(AccentAlt, QColor::fromHsl(220, 6, 24));
    map(Text, QColor::fromHsl(0, 0, 180));
    map(TextDisabled,  QColor::fromHsl(0, 0, 40));
    map(Highlight, QColor::fromHsl(216, 82, 40));
    map(Border, QColor::fromHsl(220, 3, 33));
    map(Scrollbar, QColor::fromHsl(0, 0, 70));
    map(Progress, QColor::fromHsl(216, 82, 20));
}

Stylesheet::~Stylesheet() {}

QString Stylesheet::roleName(ColorRole role) const {
    const QMetaEnum me = QMetaEnum::fromType<ColorRole>();
    return QString::fromLatin1(me.valueToKey(role)).toLower();
}

void
Stylesheet::applyQss(const QString& qss)
{
    qApp->setStyleSheet(qss);
}

bool
Stylesheet::loadQss(const QString& path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return false;
    }
    p->path = path;
    QString output = QString::fromUtf8(file.readAll());

    // replace $role placeholders
    for (auto it = p->palette.constBegin(); it != p->palette.constEnd(); ++it) {
        QString placeholder = "$" + it.key();
        QColor color = it.value();
        QString hsl = QString("hsl(%1, %2%, %3%)")
                          .arg(color.hue() == -1 ? 0 : color.hue())
                          .arg(static_cast<int>(color.hslSaturationF() * 100))
                          .arg(static_cast<int>(color.lightnessF() * 100));
        output.replace(placeholder, hsl, Qt::CaseInsensitive);
    }

    p->compiled = output;
    return true;
}

QString Stylesheet::compiled() const
{
    return p->compiled;
}

void Stylesheet::setColor(ColorRole role, const QColor& color)
{
    p->palette[roleName(role)] = color;
}

QColor Stylesheet::color(ColorRole role) const
{
    return p->palette.value(roleName(role), QColor());
}

Stylesheet* Stylesheet::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (!pi) {
        pi.reset(new Stylesheet());
    }
    return pi.data();
}
