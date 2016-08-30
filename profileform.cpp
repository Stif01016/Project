#include <QDebug>
#include <QCloseEvent>
#include <QStyleFactory>
#include <QFileDialog>
#include <QMenu>
#include <QJsonDocument>

#include "profileform.h"
#include "ui_profileform.h"
#include "defines.h"
#include "storage.h"
#include "function.h"
#include "treestyle.h"
#include "logmanager.h"

#define ID_PROFILE_INDEX        0
#define NAME_PROFILE_INDEX      1
#define FORMS_PROFILE_INDEX     2
#define OPERS_PROFILE_INDEX     3
#define PAYS_PROFILE_INDEX      4
#define REPORTS_PROFILE_INDEX   5
#define HOTKEYS_PROFILE_INDEX   6

#define KT_HOTKEY_INDEX         1

GProfileForm::GProfileForm( GStorage * st, bool select, QWidget *parent ) : QDialog( parent ),
    ui_( new Ui::ProfileForm )
{
    ui_->setupUi( this );

    storage_ = st;
    workMode_ = GProfileForm::wmView;
    formItem_ = NULL;
    operItem_ = NULL;
    payItem_ = NULL;
    requestItem_ = NULL;

    keyEdit_ = new GKeyEdit( this );
    qobject_cast< QHBoxLayout *>( ui_->keyGB->layout() )->insertWidget( 1, keyEdit_ );

    ui_->rightGB->setCurrentIndex( 0 );

    ui_->tree->setStyle( QStyleFactory::create( "windowsxp" ) );
    ui_->tree->setItemDelegate( new GItemDelegate( this ) );

    ui_->keyTree->setStyle( QStyleFactory::create( "windowsxp" ) );
    ui_->keyTree->setItemDelegate( new GItemDelegate( this ) );
    ui_->keyTree->header()->setSectionResizeMode( QHeaderView::Stretch );

    ui_->profileTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );
    ui_->profileTable->hideColumn( ID_PROFILE_INDEX );
    ui_->profileTable->hideColumn( FORMS_PROFILE_INDEX );
    ui_->profileTable->hideColumn( OPERS_PROFILE_INDEX );
    ui_->profileTable->hideColumn( PAYS_PROFILE_INDEX );
    ui_->profileTable->hideColumn( REPORTS_PROFILE_INDEX );
    ui_->profileTable->hideColumn( HOTKEYS_PROFILE_INDEX );

    contextMenu_ = new QMenu();
    addMenu_ = contextMenu_->addAction( QIcon ( ":/images/table_row_insert.png" ),
                                        ui_->addRowBtn->toolTip(),
                                        this,
                                        SLOT( onAddProfile() ) );
    copyMenu_ = contextMenu_->addAction( QIcon ( ":/images/row_copy.png" ),
                                         ui_->editRowBtn->toolTip(),
                                         this,
                                         SLOT( onAddProfile() ) );
    delMenu_ = contextMenu_->addAction( QIcon ( ":/images/table_row_delete.png" ),
                                        ui_->delRowBtn->toolTip(),
                                        this,
                                        SLOT( onDeleteProfile() ) );

    ui_->addRowBtn->setIcon( QIcon ( ":/images/table_row_insert.png" ) );
    ui_->editRowBtn->setIcon( QIcon ( ":/images/row_copy.png" ) );
    ui_->delRowBtn->setIcon( QIcon ( ":/images/table_row_delete.png" ) );
    ui_->clearBtn->setIcon( QIcon ( ":/images/clear.png" ) );
    ui_->clearBtn->setToolTip( trUtf8( "Очистить" ) );

    ui_->okBtn->setIcon( QIcon( ":/images/apply.png" ) );
    ui_->cancelBtn->setIcon( QIcon( ":/images/cancel.png" ) );

    ui_->keyGB->setEnabled( false );

    connect( ui_->addRowBtn, SIGNAL( clicked() ), this, SLOT( onAddProfile() ) );
    connect( ui_->editRowBtn, SIGNAL( clicked() ), this, SLOT( onAddProfile() ) );
    connect( ui_->delRowBtn, SIGNAL( clicked() ), this, SLOT( onDeleteProfile() ) );

    connect( ui_->okBtn, SIGNAL( clicked() ), this, SLOT( onSaveProfile() ) );
    connect( ui_->cancelBtn, SIGNAL( clicked() ), this, SLOT( onCancelProfile() ) );

    connect( ui_->profileTable, SIGNAL( currentCellChanged( int, int, int, int ) ),
             this, SLOT( onCellClicked( int, int, int, int ) ) );
    if ( select )
        connect( ui_->profileTable, SIGNAL( cellDoubleClicked( int, int ) ),
                 this, SLOT( onCellDoubleClicked( int, int ) ) );

    connect( ui_->profileEdit, SIGNAL( textEdited( QString ) ), this, SLOT( onEditProfile() ) );
    connect( ui_->tree, SIGNAL( itemChanged( QTreeWidgetItem*, int ) ), this, SLOT( onEditProfile() ) );

    connect( ui_->keyTree, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ),
             this, SLOT( onKeyTreeClicked( QTreeWidgetItem*, int ) ) );
    connect( ui_->keyTree, SIGNAL( itemChanged( QTreeWidgetItem*, int ) ), this, SLOT( onEditProfile() ) );

    connect( keyEdit_, SIGNAL( textChanged( QString ) ), this, SLOT( onKeyChanged( QString ) ) );
    connect( ui_->clearBtn, SIGNAL( clicked() ), this, SLOT( onClearKey() ) );
    connect( ui_->resetBtn, SIGNAL( clicked() ), this, SLOT( onResetKey() ) );
    connect( ui_->defKeyBtn, SIGNAL( clicked() ), this, SLOT( onDefaultKey() ) );
    connect( ui_->importBtn, SIGNAL( clicked() ), this, SLOT( onImportKey() ) );
    connect( ui_->exportBtn, SIGNAL( clicked() ), this, SLOT( onExportKey() ) );

    initRighTree();
    initKeyTree();

    showProfile( gProfileUser );
}

GProfileForm::~GProfileForm()
{
    delete ui_;
}

void GProfileForm::initRighTree()
{
    QTreeWidgetItem *item;
    QStringList dataList;
    QList< int > reqList;
    QList< int > disList;

    ui_->tree->clear();
    ui_->tree->blockSignals( true );

    formItem_ = new QTreeWidgetItem( ui_->tree, QStringList() << trUtf8( "Экраны и справочники" ) );
    foreach( int form, gFormMap.keys() ) {
        item = new QTreeWidgetItem( formItem_ );
        item->setText( 0, gFormMap[ form ] );
        item->setData( 0, Qt::UserRole, form );
        item->setCheckState( 0, Qt::Unchecked );
    }

    operItem_ = new QTreeWidgetItem( ui_->tree, QStringList() << trUtf8( "Операции" ) );
    requestItem_ = new QTreeWidgetItem( ui_->tree, QStringList() << trUtf8( "Запросы подтверждения" ) );
    discountItem_ = new QTreeWidgetItem( ui_->tree, QStringList() << trUtf8( "Скидки" ) );
    reqList << UO_REQUEST_CLEAR_CHECK << UO_REQUEST_PAY << UO_REQUEST_Z_REPORT;
    disList << UO_PROCENT_DISCOUNT << UO_SUMMA_DISCOUNT << UO_PROCENT_DISCOUNT_DOC << UO_SUMMA_DISCOUNT_DOC;
    foreach( int x, gUserOperMap.keys() ) {
        QTreeWidgetItem *parentItem;

        if ( reqList.contains( x ) )
            parentItem = requestItem_;
        else {
            if ( disList.contains( x ) )
                parentItem = discountItem_;
            else
                parentItem = operItem_;
        }
        item = new QTreeWidgetItem(  parentItem );
        item->setText( 0, gUserOperMap[ x ] );
        item->setData( 0, Qt::UserRole, x );
        item->setCheckState( 0, Qt::Unchecked );
    }

    payItem_ = new QTreeWidgetItem( ui_->tree, QStringList() << trUtf8( "Оплата" ) );
    storage_->getTableData( PAY_TYPE_TABLE );
    while ( true ) {
        dataList = storage_->getTableRowData( PAY_TYPE_TABLE );
        if ( dataList.count() == 0 )
            break;

        item = new QTreeWidgetItem( payItem_ );
        item->setText( 0, dataList[ 1 ] );
        item->setData( 0, Qt::UserRole, dataList[ 0 ] );
        item->setCheckState( 0, Qt::Unchecked );
    }

    reportItem_ = new QTreeWidgetItem( ui_->tree, QStringList() << trUtf8( "Кассовые отчеты" ) );
    foreach( int report, gReportMap.keys() ) {
        item = new QTreeWidgetItem( reportItem_ );
        item->setText( 0, gReportMap[ report ] );
        item->setData( 0, Qt::UserRole, report );
        item->setCheckState( 0, Qt::Unchecked );
    }

    ui_->tree->expandAll();
    ui_->tree->blockSignals( false );
}

void GProfileForm::initKeyTree()
{
    GHotKeyGroup *group;
    QTreeWidgetItem *item;

    ui_->keyTree->clear();
    ui_->keyTree->blockSignals( true );

    for ( int x = 0; x < gHotKeyGroupList.count(); ++x ) {
        group = gHotKeyGroupList[ x ];

        item = new QTreeWidgetItem( ui_->keyTree, QStringList() << group->name_ );
        for ( int y = 0; y < group->hKeys_.count(); ++y ) {
            QTreeWidgetItem *it;

            it = new QTreeWidgetItem( item, QStringList() << group->hKeys_[ y ]->name_
                                                          << group->hKeys_[ y ]->hotKey_ );

            it->setData( 0, Qt::UserRole, group->hKeys_[ y ]->id_ );

            //отображать в профиле настройки гор.клавиши для спец. продажи только для пользователя 'developer'
            if( group->hKeys_[ y ]->id_ == SPECIAL_MODE_HKEY && gIdUser != -1)
                it->setHidden( true );
        }
    }

    ui_->keyTree->expandAll();
    ui_->keyTree->blockSignals( false );
}

void GProfileForm::setControls()
{
    ui_->frame_2->setEnabled( ui_->profileTable->rowCount() );
    ui_->profileTable->setEnabled( workMode_ == GProfileForm::wmView );

    ui_->addRowBtn->setEnabled( workMode_ == GProfileForm::wmView );
    ui_->editRowBtn->setEnabled( workMode_ == GProfileForm::wmView && ui_->profileTable->rowCount() );
    ui_->delRowBtn->setEnabled( workMode_ == GProfileForm::wmView && ui_->profileTable->rowCount() );

    ui_->okBtn->setEnabled( workMode_ != GProfileForm::wmView );
    ui_->cancelBtn->setEnabled( workMode_ != GProfileForm::wmView );

    addMenu_->setEnabled( ui_->addRowBtn->isEnabled() );
    copyMenu_->setEnabled( ui_->editRowBtn->isEnabled() );
    delMenu_->setEnabled( ui_->delRowBtn->isEnabled() );
}

void GProfileForm::showProfile( const QString &profile )
{
    ui_->profileTable->blockSignals( true );
    storage_->showTableInTable( USERS_PROFILE_TABLE, ui_->profileTable );

    if ( profile != "" )
        for ( int x = 0; x < ui_->profileTable->rowCount(); ++x ) {
            if ( ui_->profileTable->item( x, NAME_PROFILE_INDEX )->text() == profile ) {
                ui_->profileTable->selectRow( x );
                break;
            }
        }
    else
        if ( ui_->profileTable->rowCount() )
            ui_->profileTable->selectRow( 0 );

    onCellClicked( ui_->profileTable->currentRow(), 0 );
    ui_->profileTable->blockSignals( false );

    setControls();
}

void GProfileForm::onCellClicked( int row, int, int, int )
{
    clearEdits();
    QVariantMap jMap;
    Qt::CheckState chState;

    if ( row < 0 )
        return;

    ui_->profileEdit->setText( ui_->profileTable->item( row, NAME_PROFILE_INDEX )->text() );
    ui_->profileEdit->setProperty( "profile", ui_->profileTable->item( row, NAME_PROFILE_INDEX )->text() );

    ui_->tree->setCurrentItem( NULL );
    ui_->tree->blockSignals( true );

    // экраны
    jMap = QJsonDocument::fromJson( ui_->profileTable->item( row, FORMS_PROFILE_INDEX )->text().toLocal8Bit() ).toVariant().toMap();
    for ( int x = 0; x < formItem_->childCount(); ++x ) {
        if ( jMap.contains( formItem_->child( x )->data( 0, Qt::UserRole ).toString() ) == true )
            chState = jMap[ formItem_->child( x )->data( 0, Qt::UserRole ).toString() ].toBool() ? Qt::Checked : Qt::Unchecked;
        else
            chState = Qt::Checked;
        formItem_->child( x )->setCheckState( 0, chState );
    }

    // операции
    jMap = QJsonDocument::fromJson( ui_->profileTable->item( row, OPERS_PROFILE_INDEX )->text().toLocal8Bit() ).toVariant().toMap();
    for ( int x = 0; x < operItem_->childCount(); ++x ) {
        if ( jMap.contains( operItem_->child( x )->data( 0, Qt::UserRole ).toString() ) == true )
            chState = jMap[ operItem_->child( x )->data( 0, Qt::UserRole ).toString() ].toBool() ? Qt::Checked : Qt::Unchecked;
        else {
            int op = operItem_->child( x )->data( 0, Qt::UserRole ).toInt();
            chState = ( op != UO_SALE_GROUP &&
                        op != UO_SORT_RECEIPT_COLUMN &&
                        op != UO_FILTER_DISABLED_ART ) ? Qt::Checked : Qt::Unchecked;
        }
        operItem_->child( x )->setCheckState( 0, chState );
    }

    // запросы подтверждения
    for ( int x = 0; x < requestItem_->childCount(); ++x ) {
        if ( jMap.contains( requestItem_->child( x )->data( 0, Qt::UserRole ).toString() ) == true )
            chState = jMap[ requestItem_->child( x )->data( 0, Qt::UserRole ).toString() ].toBool() ? Qt::Checked : Qt::Unchecked;
        else
            chState = Qt::Checked;
        requestItem_->child( x )->setCheckState( 0, chState );
    }

    // скидки
    for ( int x = 0; x < discountItem_->childCount(); ++x ) {
        if ( jMap.contains( discountItem_->child( x )->data( 0, Qt::UserRole ).toString() ) == true )
            chState = jMap[ discountItem_->child( x )->data( 0, Qt::UserRole ).toString() ].toBool() ? Qt::Checked : Qt::Unchecked;
        else
            chState = Qt::Checked;
        discountItem_->child( x )->setCheckState( 0, chState );
    }

    // оплата
    jMap = QJsonDocument::fromJson( ui_->profileTable->item( row, PAYS_PROFILE_INDEX )->text().toLocal8Bit() ).toVariant().toMap();
    for ( int x = 0; x < payItem_->childCount(); ++x ) {
        if ( jMap.contains( payItem_->child( x )->data( 0, Qt::UserRole ).toString() ) == true )
            chState = jMap[ payItem_->child( x )->data( 0, Qt::UserRole ).toString() ].toBool() ? Qt::Checked : Qt::Unchecked;
        else
            chState = Qt::Checked;
        payItem_->child( x )->setCheckState( 0, chState );
    }

    // кассовые отчеты
    jMap = QJsonDocument::fromJson( ui_->profileTable->item( row, REPORTS_PROFILE_INDEX )->text().toLocal8Bit() ).toVariant().toMap();
    for ( int x = 0; x < reportItem_->childCount(); ++x ) {
        if ( jMap.contains( reportItem_->child( x )->data( 0, Qt::UserRole ).toString() ) == true )
            chState = jMap[ reportItem_->child( x )->data( 0, Qt::UserRole ).toString() ].toBool() ? Qt::Checked : Qt::Unchecked;
        else
            chState = Qt::Checked;
        reportItem_->child( x )->setCheckState( 0, chState );
    }

    // горячие клавиши
    ui_->keyTree->setCurrentItem( NULL );
    ui_->keyTree->blockSignals( true );

    if ( ui_->profileTable->item( row, HOTKEYS_PROFILE_INDEX ) )
        setKeyMap( QJsonDocument::fromJson( ui_->profileTable->item( row, HOTKEYS_PROFILE_INDEX )->text().toLocal8Bit() ).toVariant().toMap() );
    ui_->keyTree->blockSignals( false );

    ui_->tree->blockSignals( false );
}

void GProfileForm::clearEdits()
{
    ui_->profileEdit->clear();
    ui_->tree->blockSignals( true );
    ui_->keyTree->blockSignals( true );

    for ( int x = 0; x < formItem_->childCount(); ++x )
        formItem_->child( x )->setCheckState( 0, Qt::Checked );

    for ( int x = 0; x < operItem_->childCount(); ++x )
        operItem_->child( x )->setCheckState( 0, Qt::Checked );
    for ( int x = 0; x < requestItem_->childCount(); ++x )
        requestItem_->child( x )->setCheckState( 0, Qt::Checked );
    for ( int x = 0; x < discountItem_->childCount(); ++x )
        discountItem_->child( x )->setCheckState( 0, Qt::Checked );

    for ( int x = 0; x < payItem_->childCount(); ++x )
        payItem_->child( x )->setCheckState( 0, Qt::Checked );

    onDefaultKey();

    ui_->keyTree->blockSignals( false );
    ui_->tree->blockSignals( false );
}

void GProfileForm::onAddProfile()
{
    workMode_ = GProfileForm::wmAdd;

    if ( sender() == ui_->addRowBtn ||
         sender() == addMenu_ )
        clearEdits();
    else
        ui_->profileEdit->setText( ui_->profileEdit->text() + "_1" );

    setControls();
    ui_->frame_2->setEnabled( true );
    ui_->profileEdit->setFocus();
}

void GProfileForm::onEditProfile()
{
    if ( ui_->profileTable->currentRow() < 0 )
        return;

    if ( workMode_ == GProfileForm::wmView ) {
        workMode_ = GProfileForm::wmEdit;
        setControls();
    }
}

void GProfileForm::onDeleteProfile()
{
    QVariantMap vMap;

    if ( ui_->profileTable->currentRow() < 0 )
        return;

    if ( showQuestion( trUtf8( "Удаление" ), trUtf8( "Удалить текущий профиль?" ), this ) == false )
        return;

    vMap[ ID_USER_PROFILE ] = ui_->profileTable->item( ui_->profileTable->currentRow(), ID_PROFILE_INDEX )->text();
    if ( storage_->deleteTableRowData( USERS_PROFILE_TABLE, vMap, false ) == true ) {
        gLogManager->addUserMessage( trUtf8( "Удаление профиля пользователя " ) + ui_->profileEdit->text()  );
        showProfile();
    }
    else
        showError( trUtf8( "Ошибка" ), 183, trUtf8( "Ошибка удаления профиля пользователя. Попытка удаления профиля существующего пользователя." ), this );
}

QVariantMap GProfileForm::getKeyMap()
{
    QVariantMap kMap;

    for( int x = 0; x < ui_->keyTree->topLevelItemCount(); ++x ) {
        for( int y = 0; y < ui_->keyTree->topLevelItem( x )->childCount(); ++y )
            kMap[ ui_->keyTree->topLevelItem( x )->child( y )->data( 0, Qt::UserRole ).toString() ] = ui_->keyTree->topLevelItem( x )->child( y )->text( KT_HOTKEY_INDEX );
    }

    return kMap;
}

void GProfileForm::setKeyMap( QVariantMap kMap )
{
    if ( kMap.count() == 0 )
        return;

    for( int x = 0; x < ui_->keyTree->topLevelItemCount(); ++x ) {
        for( int y = 0; y < ui_->keyTree->topLevelItem( x )->childCount(); ++y ){
            QString key;

            if ( kMap.contains( ui_->keyTree->topLevelItem( x )->child( y )->data( 0, Qt::UserRole ).toString() ) == true )
                key = kMap[ ui_->keyTree->topLevelItem( x )->child( y )->data( 0, Qt::UserRole ).toString() ].toString();
            else
                key = findHotKey( ui_->keyTree->topLevelItem( x )->child( y )->data( 0, Qt::UserRole ).toInt(), true  );
            ui_->keyTree->topLevelItem( x )->child( y )->setText( KT_HOTKEY_INDEX, key );
        }
    }
}

void GProfileForm::onSaveProfile()
{
    QVariantList vList;
    QVariantMap vMap;
    QVariantMap jMap;

    if ( ui_->profileTable->currentRow() < 0 && workMode_ == GProfileForm::wmEdit )
        return;

    // проверка на правильнось вводимых значений
    if ( ui_->profileEdit->text() == "" ) {
        showWarning( workMode_ == GProfileForm::wmAdd ? trUtf8( "Добавление" ) : trUtf8( "Редактирование" ),
                     trUtf8( "Профиль не может быть пустым" ),
                     this );
        return;
    }

    vMap[ NAME_USER_PROFILE ] = ui_->profileEdit->text().replace( "'", "''" );
    vList << vMap;
    if (ui_->profileEdit->property( "profile" ).toString() == gProfileUser )
        gProfileUser = ui_->profileEdit->text();

    // экраны
    for ( int x = 0; x < formItem_->childCount(); ++x )
        jMap[ formItem_->child( x )->data( 0, Qt::UserRole ).toString() ] = formItem_->child( x )->checkState( 0 ) == Qt::Checked;

    vMap[ FORMS_PROFILE ] = QJsonDocument::fromVariant( jMap ).toJson();
    vList << vMap;

    // операции
    jMap.clear();
    for ( int x = 0; x < operItem_->childCount(); ++x )
        jMap[ operItem_->child( x )->data( 0, Qt::UserRole ).toString() ] = operItem_->child( x )->checkState( 0 ) == Qt::Checked;

    // запросы подтверждения
    for ( int x = 0; x < requestItem_->childCount(); ++x )
        jMap[ requestItem_->child( x )->data( 0, Qt::UserRole ).toString() ] = requestItem_->child( x )->checkState( 0 ) == Qt::Checked;

    // скидки
    for ( int x = 0; x < discountItem_->childCount(); ++x )
        jMap[ discountItem_->child( x )->data( 0, Qt::UserRole ).toString() ] = discountItem_->child( x )->checkState( 0 ) == Qt::Checked;

    vMap.clear();
    vMap[ OPERS_PROFILE ] = QJsonDocument::fromVariant( jMap ).toJson();
    vList << vMap;

    // оплата
    jMap.clear();
    for ( int x = 0; x < payItem_->childCount(); ++x )
        jMap[ payItem_->child( x )->data( 0, Qt::UserRole ).toString() ] =  payItem_->child( x )->checkState( 0 ) == Qt::Checked;

    vMap.clear();
    vMap[ PAYS_PROFILE ] = QJsonDocument::fromVariant( jMap ).toJson();
    vList << vMap;

    // кассовые отчеты
    jMap.clear();
    for ( int x = 0; x < reportItem_->childCount(); ++x )
        jMap[ reportItem_->child( x )->data( 0, Qt::UserRole ).toString() ] = reportItem_->child( x )->checkState( 0 ) == Qt::Checked;

    vMap.clear();
    vMap[ REPORTS_PROFILE ] = QJsonDocument::fromVariant( jMap ).toJson();
    vList << vMap;

    // горячие клавиши
    vMap.clear();
    vMap[ HOTKEYS_PROFILE ] = QJsonDocument::fromVariant( getKeyMap() ).toJson();
    vList << vMap;

    if ( workMode_ == GProfileForm::wmAdd )
        storage_->addTableRowData( USERS_PROFILE_TABLE, vList );

    if ( workMode_ == GProfileForm::wmEdit ) {
        vMap.clear();
        vMap[ ID_USER_PROFILE ] = ui_->profileTable->item( ui_->profileTable->currentRow(), ID_PROFILE_INDEX )->text();
        storage_->updateTableRowData( USERS_PROFILE_TABLE, vList, vMap );
    }

    workMode_ = GProfileForm::wmView;
    showProfile( ui_->profileEdit->text() );
}

void GProfileForm::onCancelProfile()
{
    workMode_ = GProfileForm::wmView;
    setControls();

    onCellClicked( ui_->profileTable->currentRow(), 0 );
}

void GProfileForm::closeEvent( QCloseEvent *e )
{
    if ( workMode_ != GProfileForm::wmView ) {
        if ( showQuestion( trUtf8( "Профили" ), trUtf8( "Сохранить изменения?" ), this ) == true ) {
            onSaveProfile();
            if ( workMode_ != GProfileForm::wmView )
                e->ignore();
        }
    }
    else
        e->accept();
}

void GProfileForm::keyPressEvent( QKeyEvent *e )
{
    if ( !e->modifiers() || ( e->modifiers() & Qt::KeypadModifier && e->key() == Qt::Key_Enter ) ) {
        switch ( e->key() ) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if ( workMode_ != GProfileForm::wmView )
                onSaveProfile();
            break;
        case Qt::Key_Escape:
            if ( workMode_ != GProfileForm::wmView )
                onCancelProfile();
            else
               QDialog::keyPressEvent( e );
            return;
        default:
            e->ignore();
            return;
        }
    }
    else
        e->ignore();
}

void GProfileForm::onKeyTreeClicked( QTreeWidgetItem *item, int )
{
    if ( item == NULL )
        return;

    ui_->keyGB->setEnabled( item->childCount() == 0 );
    keyEdit_->setText( item->text( KT_HOTKEY_INDEX ) );
    ui_->clearBtn->setVisible( keyEdit_->text() != "" );
}

void GProfileForm::onKeyChanged( const QString &text )
{
    if ( ui_->keyTree->currentItem() == NULL ||
         ui_->keyTree->currentItem()->data( 0, Qt::UserRole ).isValid() == false )
        return;

    ui_->keyTree->currentItem()->setText( KT_HOTKEY_INDEX, text );
    ui_->clearBtn->setVisible( keyEdit_->text() != "" );
}

void GProfileForm::onClearKey()
{
    if ( ui_->keyTree->currentItem() == NULL ||
         ui_->keyTree->currentItem()->data( 0, Qt::UserRole ).isValid() == false )
        return;

    keyEdit_->clear();
}

void GProfileForm::onResetKey()
{
    QVariantMap kMap;

    if ( ui_->keyTree->currentItem() == NULL ||
         ui_->keyTree->currentItem()->data( 0, Qt::UserRole ).isValid() == false )
        return;

    kMap = BA2Variant( qUncompress( QByteArray::fromHex( ui_->profileTable->item( ui_->profileTable->currentRow(), HOTKEYS_PROFILE_INDEX )->text().toLocal8Bit() ) ) ).toMap();

    keyEdit_->setText( kMap[ ui_->keyTree->currentItem()->data( 0, Qt::UserRole ).toString() ].toString() );
}

void GProfileForm::onDefaultKey()
{
    for( int x = 0; x < ui_->keyTree->topLevelItemCount(); ++x ) {
        for( int y = 0; y < ui_->keyTree->topLevelItem( x )->childCount(); ++y )
            ui_->keyTree->topLevelItem( x )->child( y )->setText( KT_HOTKEY_INDEX,
                                                                  findHotKey( ui_->keyTree->topLevelItem( x )->child( y )->data( 0, Qt::UserRole ).toInt(), true ) );
    }
}

void GProfileForm::onExportKey()
{
    QString fileName;

    fileName = QFileDialog::getSaveFileName( this, trUtf8( "Экспорт..." ), getINIValue( HOTKEYS_INI_SECTION, HOTKEYS_EXP_PATH ).toString(), trUtf8( "Файл горячих клавиш ( *.fhk )" ) );

    if ( fileName == "" )
        return;
    else
        setINIValue( HOTKEYS_INI_SECTION, HOTKEYS_EXP_PATH, fileName );

    QFile file( fileName );
    if ( file.open( QIODevice::WriteOnly ) ) {
        file.write( Variant2BA( getKeyMap() ) );
        file.close();
    }
}

void GProfileForm::onImportKey()
{
    QString fileName;

    fileName = QFileDialog::getOpenFileName( this, trUtf8( "Импорт..." ), getINIValue( HOTKEYS_INI_SECTION, HOTKEYS_IMP_PATH ).toString(), trUtf8( "Файл горячих клавиш ( *.fhk )" ) );

    if ( fileName == "" )
        return;
    else
        setINIValue( HOTKEYS_INI_SECTION, HOTKEYS_IMP_PATH, fileName );

    gLogManager->addUserMessage( trUtf8( "Импорт горячих клавиш из файла" ) + " " + fileName );
    QFile file( fileName );
    if ( file.open( QIODevice::ReadOnly ) ) {
        QVariantMap kMap;

        kMap = BA2Variant( file.readAll() ).toMap();
        file.close();

        if ( kMap.count() != 0 ) {
            setKeyMap( kMap );
            gLogManager->addUserMessage( trUtf8( "Импорт горячих клавиш осуществлен успешно" ) );
        }
        else {
            gLogManager->addUserMessage( trUtf8( "Ошибка при импорте горячих клавиш" ) );
//            gLogManager->addPortalErrorMessage( trUtf8( "Неверный формат файла" ) );
            showError( trUtf8( "Ошибка импорта" ), 70, trUtf8( "Неверный формат файла импорта горячих клавиш" ), this );
        }
    }
}

void GProfileForm::onCellDoubleClicked ( int row, int column )
{
    Q_UNUSED( row )
    Q_UNUSED( column )

    accept();
}

QVariant GProfileForm::getSelectedProfile()
{
    if ( ui_->profileTable->currentRow() < 0 )
        return QVariant();

    return QVariant( ui_->profileTable->item( ui_->profileTable->currentRow(), 0 )->text() );
}

void GProfileForm::contextMenuEvent ( QContextMenuEvent *event )
{
    if ( contextMenu_ ) {
        contextMenu_->exec( event->globalPos() );
    }
}

//=============================================================================
// class GKeyEdit
//=============================================================================
void GKeyEdit::keyPressEvent( QKeyEvent *e )
{
    bool isShiftPressed = e->modifiers() & Qt::ShiftModifier;
    bool isControlPressed = e->modifiers() & Qt::ControlModifier;
    bool isAltPressed = e->modifiers() & Qt::AltModifier;
    bool isMetaPressed = e->modifiers() & Qt::MetaModifier;
    int result = 0;
    if( isControlPressed )
      result += Qt::CTRL;
    if( isAltPressed )
      result += Qt::ALT;
    if( isShiftPressed )
      result += Qt::SHIFT;
    if( isMetaPressed )
      result += Qt::META;

    int aKey = e->key();

    if ( isValidKey( aKey ) ) {
        result += e->key();

        QKeySequence seq( result );
        if ( text().split( ", " ).contains( seq.toString() ) == false )
            setText( ( text() == "" ? "" : text() + ", " ) + seq.toString() );
    }
}

bool GKeyEdit::isValidKey( int aKey )
{
   if ( aKey == Qt::Key_Underscore || aKey == Qt::Key_Escape ||
      ( aKey >= Qt::Key_Backspace && aKey <= Qt::Key_Delete ) ||
      ( aKey >= Qt::Key_Home && aKey <= Qt::Key_PageDown ) ||
      ( aKey >= Qt::Key_F1 && aKey <= Qt::Key_F12 )  ||
      ( aKey >= Qt::Key_Space && aKey <= Qt::Key_Asterisk ) ||
      ( aKey >= Qt::Key_Plus && aKey <= Qt::Key_Question ) ||
      ( aKey >= Qt::Key_A && aKey <= Qt::Key_AsciiTilde ) )
     return true;
   return false;
}
