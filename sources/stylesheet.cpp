#include "stylesheet.h"
#include "icctransform.h"
#include <QApplication>
#include <QFile>
#include <QMetaEnum>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>

QScopedPointer<Stylesheet, Stylesheet::Deleter> Stylesheet::pi;

class StylesheetPrivate : public QObject {
    Q_OBJECT
public:
    StylesheetPrivate();
    ~StylesheetPrivate();
    QString roleName(Stylesheet::ColorRole role) const;
    QString roleName(Stylesheet::FontRole role) const;

    QString path;
    QString compiled;
    QHash<QString, QColor> palette;
    QHash<QString, int> fonts;
};

StylesheetPrivate::StylesheetPrivate() {}
StylesheetPrivate::~StylesheetPrivate() {}

QString
StylesheetPrivate::roleName(Stylesheet::ColorRole role) const
{
    const QMetaEnum me = QMetaEnum::fromType<Stylesheet::ColorRole>();
    return QString::fromLatin1(me.valueToKey(role)).toLower();
}

QString
StylesheetPrivate::roleName(Stylesheet::FontRole role) const
{
    const QMetaEnum me = QMetaEnum::fromType<Stylesheet::FontRole>();
    return QString::fromLatin1(me.valueToKey(role)).toLower();
}

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
    map(TextDisabled, QColor::fromHsl(0, 0, 40));
    map(Highlight, QColor::fromHsl(216, 82, 40));
    map(Border, QColor::fromHsl(220, 3, 33));
    map(Scrollbar, QColor::fromHsl(0, 0, 70));
    map(Progress, QColor::fromHsl(216, 82, 20));

    setFontSize(DefaultSize, 11);
    setFontSize(SmallSize, 9);
    setFontSize(LargeSize, 14);
}

Stylesheet::~Stylesheet() {}

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

    for (auto it = p->palette.constBegin(); it != p->palette.constEnd(); ++it) {
        QString placeholder = "$" + it.key();
        QColor color = it.value();
        QString hsl = QString("hsl(%1, %2%, %3%)")
                          .arg(color.hue() == -1 ? 0 : color.hue())
                          .arg(static_cast<int>(color.hslSaturationF() * 100))
                          .arg(static_cast<int>(color.lightnessF() * 100));
        output.replace(placeholder, hsl, Qt::CaseInsensitive);
    }

    for (auto it = p->fonts.constBegin(); it != p->fonts.constEnd(); ++it) {
        QString placeholder = "$" + it.key();
        output.replace(placeholder, QString::number(it.value()) + "px", Qt::CaseInsensitive);
    }

    p->compiled = output;
    return true;
}

QString
Stylesheet::compiled() const
{
    return p->compiled;
}

void
Stylesheet::setColor(ColorRole role, const QColor& color)
{
    p->palette[p->roleName(role)] = color;
}

QColor
Stylesheet::color(ColorRole role) const
{
    return p->palette.value(p->roleName(role), QColor());
}

void
Stylesheet::setFontSize(FontRole role, int size)
{
    p->fonts[p->roleName(role)] = size;
}

int
Stylesheet::fontSize(FontRole role) const
{
    return p->fonts.value(p->roleName(role), -1);
}

Stylesheet*
Stylesheet::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (!pi) {
        pi.reset(new Stylesheet());
    }
    return pi.data();
}
