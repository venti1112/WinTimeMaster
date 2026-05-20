#ifndef ABOUTCONTROLLER_H
#define ABOUTCONTROLLER_H

#include <QObject>

class AboutController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString compiler READ compiler CONSTANT)
    Q_PROPERTY(QString buildType READ buildType CONSTANT)
    Q_PROPERTY(QString developers READ developers CONSTANT)
    Q_PROPERTY(QString githubUrl READ githubUrl CONSTANT)

public:
    explicit AboutController(QObject *parent = nullptr);

    QString appVersion() const;
    QString qtVersion() const;
    QString compiler() const;
    QString buildType() const;
    QString developers() const;
    QString githubUrl() const;
};

#endif // ABOUTCONTROLLER_H
