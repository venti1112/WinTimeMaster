#ifndef TIMERULEMODEL_H
#define TIMERULEMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include "TimeRule.h"

class TimeRuleModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        StartTimeRole,
        EndTimeRole,
        RepeatModeRole,
        WeekDaysRole,
        EnabledRole
    };
    Q_ENUM(Roles)

    explicit TimeRuleModel(QObject *parent = nullptr);
    ~TimeRuleModel() override = default;

    // QAbstractListModel 接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    // QML 可调用的方法
    Q_INVOKABLE void addRule();
    Q_INVOKABLE void updateRule(int row, const QVariantMap &data);
    Q_INVOKABLE void removeRule(int row);

    // 辅助方法
    TimeRule *ruleAt(int row) const;
    QList<TimeRule*> rules() const;

private:
    void loadRules();
    void saveRules();
    int nextRuleId();

    QList<TimeRule*> m_rules;
    int m_nextId = 1;
};

#endif // TIMERULEMODEL_H