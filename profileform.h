#ifndef PROFILEFORM_H
#define PROFILEFORM_H

#include <QDialog>
#include <QVariant>
#include <QLineEdit>

namespace Ui {
    class ProfileForm;
}

class GStorage;
class QTreeWidgetItem;
class GKeyEdit;
class QMenu;

class GProfileForm : public QDialog
{
    Q_OBJECT

public:
    enum WorkMode {
        wmView,
        wmAdd,
        wmEdit
    };

    GProfileForm( GStorage * st, bool select = false, QWidget *parent = 0 );
    ~GProfileForm();

    QVariant getSelectedProfile();

private slots:
    void onAddProfile();
    void onEditProfile();
    void onDeleteProfile();

    void onSaveProfile();
    void onCancelProfile();

    void onCellClicked( int row, int column, int previousRow = -1, int previousColumn = -1 );
    void onCellDoubleClicked ( int row, int column );

    void onKeyTreeClicked( QTreeWidgetItem *item, int );
    void onKeyChanged( const QString &text );
    void onClearKey();
    void onResetKey();
    void onDefaultKey();
    void onExportKey();
    void onImportKey();

protected:
    void closeEvent( QCloseEvent *e );
    void keyPressEvent( QKeyEvent *e );
    void contextMenuEvent ( QContextMenuEvent *event );

private:
    Ui::ProfileForm *ui_;
    GStorage *storage_;
    WorkMode workMode_;
    GKeyEdit *keyEdit_;
    QMenu *contextMenu_;
    QAction *addMenu_;
    QAction *copyMenu_;
    QAction *delMenu_;

    QTreeWidgetItem *formItem_;
    QTreeWidgetItem *operItem_;
    QTreeWidgetItem *payItem_;
    QTreeWidgetItem *reportItem_;
    QTreeWidgetItem *requestItem_;
    QTreeWidgetItem *discountItem_;

    void clearEdits();
    void setControls();
    void showProfile( const QString &profile = "" );

    void initRighTree();
    void initKeyTree();

    QVariantMap getKeyMap();
    void setKeyMap( QVariantMap kMap );
};

class GKeyEdit : public QLineEdit
{
    Q_OBJECT

public:
    GKeyEdit( QWidget *parent = 0 ) : QLineEdit( parent ) {}

protected:
    void keyPressEvent( QKeyEvent *event );

private:
    bool isValidKey( int );
};

#endif // PROFILEFORM_H
