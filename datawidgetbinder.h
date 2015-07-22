#ifndef DATAWIDGETBINDER_H
#define DATAWIDGETBINDER_H

#include <QObject>
#include <QMap>
#include <QAbstractItemModel>

class DataWidgetBinder : public QObject
{
		Q_OBJECT
	public:
		struct Binding {
				QWidget *mWidget;
				QString mProperty;
				int mColumn;
				QString mNotifySignal;
		};

		explicit DataWidgetBinder(QObject *parent = 0);

		QAbstractItemModel *model() const;
		void setModel(QAbstractItemModel *model);

		QModelIndex currentIndex() const;
		void setCurrentIndex(QModelIndex index);

		void addBinding(QWidget *widget, QString property, int column, QString notifySignal = QString());
		void removeBinding(QWidget *widget);

	private slots:
		void propertyChanged();
		void dataChanged(QModelIndex topLeft, QModelIndex bottomRight);

	private:
		void updateValues();
		QString signature(QMetaMethod metaMethod);

		QAbstractItemModel *mModel;
		QModelIndex mCurrentIndex;

		QMap<QWidget*, Binding> mBindings;
		bool mEditingWidget;
		bool mEditingModel;
};

#endif // DATAWIDGETBINDER_H
