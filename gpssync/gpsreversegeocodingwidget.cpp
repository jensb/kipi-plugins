/* ============================================================
 *
 * Date        : 2010-03-26
 * Description : A widget to configure the GPS correlation
 *
 * Copyright (C) 2010 by Michael G. Hansen <mike at mghansen dot de>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "gpsreversegeocodingwidget.moc"

// Qt includes

//#include <QItemSelectionModel>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QAbstractItemModel>
#include <QTreeView>
#include <QContextMenuEvent>

// KDE includes

#include <kaction.h>
#include <kconfiggroup.h>
#include <khtml_part.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kvbox.h>
#include <kcombobox.h>
#include <kseparator.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kmenu.h>

// Libkmap includes

#include <libkmap/html_widget.h>
#include <libkmap/kmap_primitives.h>

//local includes

#include "gpssyncdialog.h"
#include "kipiimagemodel.h"
#include "gpsimageitem.h"
#include "gpsreversegeocodingwidget.h"
#include "backend-rg.h"
#include "backend-geonames-rg.h"
#include "backend-osm-rg.h"
#include "backend-geonamesUS-rg.h"
#include "parseTagString.h"
#include "rgtagmodel.h"
#include "tests/simpletreemodel/simpletreemodel.h"

#include <libkipi/interface.h>
#include <libkipi/imagecollection.h>
#include <libkipi/version.h>

#ifdef GPSSYNC2_MODELTEST
#include <modeltest.h>
#endif /* GPSSYNC2_MODELTEST */

namespace KIPIGPSSyncPlugin
{

class GPSReverseGeocodingWidgetPrivate
{
public:
    GPSReverseGeocodingWidgetPrivate()
    {
    }

    bool hideOptions;
    bool UIEnabled;
    QLabel *label;
    KipiImageModel* imageModel;
    QItemSelectionModel* selectionModel;
    QPushButton* buttonRGSelected;
    
    KComboBox* serviceComboBox;
    KComboBox *languageEdit;
    QList<RGInfo> photoList;
    QList<RGBackend*> backendRGList;
    RGBackend* currentBackend;
    int requestedRGCount;
    int receivedRGCount;
    QLineEdit* baseTagEdit;
    QPushButton* buttonHideOptions;
    QCheckBox *autoTag;
    QCheckBox *iptc, *xmpLoc, *xmpKey;
    QWidget* UGridContainer;
    QWidget* LGridContainer;
    QLabel* baseTagLabel;
    QLabel* serviceLabel;
    QLabel* metadataLabel;
    QLabel* languageLabel;
    int backendIndex;

    QAbstractItemModel* externTagModel;
    RGTagModel* tagModel;
    QTreeView *tagTreeView;

    QItemSelectionModel* tagSelectionModel; 
    KAction* actionAddCountry;
    KAction* actionAddCity;
};


GPSReverseGeocodingWidget::GPSReverseGeocodingWidget(KIPI::Interface* interface, KipiImageModel* const imageModel, QItemSelectionModel* const selectionModel, QWidget *const parent)
: QWidget(parent), d(new GPSReverseGeocodingWidgetPrivate())
{

    d->imageModel = imageModel;
    d->selectionModel = selectionModel;
    
    d->UIEnabled = true;
    // we need to have a main layout and add KVBox to it or derive from KVBox
    // - or is there an easier way to use KVBox?
    QVBoxLayout* vBoxLayout = new QVBoxLayout(this);

    KVBox* const vbox = new KVBox(this);
    vBoxLayout->addWidget(vbox);
    vbox->layout()->setSpacing(0);
    vbox->layout()->setMargin(0);
    

    d->UGridContainer = new QWidget(vbox);
    d->tagTreeView = new QTreeView(vbox);
    Q_ASSERT(d->tagTreeView!=0);

    // TODO: workaround until the new libkipi hits the streets
#if KIPI_VERSION>=0x010109
    d->externTagModel = interface->getTagTree();
#endif

    if (!d->externTagModel)
    {
        // we currently require a backend model, even if it is empty
        d->externTagModel = new SimpleTreeModel(1, this);
    }

    if (d->externTagModel)
    {
        d->tagModel = new RGTagModel(d->externTagModel, this);
        d->tagTreeView->setModel(d->tagModel);

#ifdef GPSSYNC2_MODELTEST
         new ModelTest(d->externTagModel, d->tagTreeView);
         new ModelTest(d->tagModel, d->tagTreeView);
#endif /* GPSSYNC2_MODELTEST */
    }


    d->tagSelectionModel = new QItemSelectionModel(d->tagModel);
    d->tagTreeView->setSelectionModel(d->tagSelectionModel);
    //d->tagModel->setSelectionModel(d->tagsSelectionModel);

    d->actionAddCountry = new KAction(i18n("Add country tags"), this);
    d->actionAddCity = new KAction(i18n("Add city tags"), this);

    QGridLayout* const gridLayout = new QGridLayout(d->UGridContainer);

    d->languageLabel = new QLabel(i18n("Select language:"), d->UGridContainer);
    d->languageEdit = new KComboBox(d->UGridContainer);

    d->languageEdit->addItem(i18n("English"),"en");
    d->languageEdit->addItem(i18n("German"), "de");
    d->languageEdit->addItem(i18n("Romanian"), "ro");
    d->languageEdit->addItem(i18n("Chinese"), "zh");
    d->languageEdit->addItem(i18n("Arabic"), "ar");
    d->languageEdit->addItem(i18n("Morroco"), "ma");
    d->languageEdit->addItem(i18n("Egiptian"), "eg");

    d->serviceLabel = new QLabel(i18n("Select service:"), d->UGridContainer);
    d->serviceComboBox = new KComboBox(d->UGridContainer);

    d->serviceComboBox->addItem(i18n("Geonames.org place name (non-US)"));
    d->serviceComboBox->addItem(i18n("Geonames.org full address (US only)"));
    d->serviceComboBox->addItem(i18n("Open Street Map"));


    d->baseTagLabel = new QLabel(i18n("Select base tag:"), d->UGridContainer);
    d->baseTagEdit = new QLineEdit("My Tags/{Country}/{City}", d->UGridContainer);



    int row = 0;
    gridLayout->addWidget(d->serviceLabel,row,0,1,2);
    row++;
    gridLayout->addWidget(d->serviceComboBox,row,0,1,2); 
    row++;
    gridLayout->addWidget(d->languageLabel,row,0,1,1);
    gridLayout->addWidget(d->languageEdit,row,1,1,1);
    row++;
    gridLayout->addWidget(d->baseTagLabel,row,0,1,2);
    row++;
    gridLayout->addWidget(d->baseTagEdit, row, 0,1,2);

    d->UGridContainer->setLayout(gridLayout);

    KSeparator* const separator = new KSeparator(Qt::Horizontal, vbox);
    d->buttonHideOptions = new QPushButton(i18n("Less options"), vbox);
    d->hideOptions = true;


    d->LGridContainer = new QWidget(vbox);
    QGridLayout* LGridLayout = new QGridLayout(d->LGridContainer);
       
    d->autoTag = new QCheckBox("Tag automatically when coordinates are changed", d->LGridContainer);

    d->metadataLabel = new QLabel( i18n("Write tags to:"),d->LGridContainer); 

    d->iptc = new QCheckBox( i18n("IPTC"), d->LGridContainer);
    d->xmpLoc = new QCheckBox( i18n("XMP location"), d->LGridContainer);
    d->xmpKey = new QCheckBox( i18n("XMP keywords"), d->LGridContainer);

    row = 0;
    LGridLayout->addWidget(d->autoTag, row,0,1,3);
    row++;
    LGridLayout->addWidget(d->metadataLabel, row,0,1,3);
    row++;
    LGridLayout->addWidget(d->iptc,row,0,1,3);
    row++;
    LGridLayout->addWidget(d->xmpLoc,row,0,1,3);
    row++;
    LGridLayout->addWidget(d->xmpKey, row,0,1,3);

    d->LGridContainer->setLayout(LGridLayout);

    d->buttonRGSelected = new QPushButton(i18n("Apply reverse geocoding"), vbox);

    dynamic_cast<QVBoxLayout*>(vbox->layout())->addStretch(300); 

    //d->backendRGList.append(new BackendGoogleRG(this));
    d->backendRGList.append(new BackendGeonamesRG(this));
    d->backendRGList.append(new BackendGeonamesUSRG(this));
    d->backendRGList.append(new BackendOsmRG(this));
   
    d->tagTreeView->installEventFilter(this);
 
    updateUIState();

    connect(d->buttonRGSelected, SIGNAL(clicked()),
            this, SLOT(slotButtonRGSelected()));

    connect(d->buttonHideOptions, SIGNAL(clicked()),
            this, SLOT(slotHideOptions()));

    connect(d->selectionModel, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection)),
            this, SLOT(updateUIState()));

    connect(d->tagTreeView, SIGNAL( clicked(const QModelIndex &)), 
            this, SLOT( treeItemClicked(const QModelIndex &)));    

    connect(d->actionAddCountry, SIGNAL(triggered(bool)),
            this, SLOT(slotAddCountry()));
    
    connect(d->actionAddCity, SIGNAL(triggered(bool)),
            this, SLOT(slotAddCity()));

    for (int i=0; i<d->backendRGList.count(); ++i)
    {
        connect(d->backendRGList[i], SIGNAL(signalRGReady(QList<RGInfo> &)),
                this, SLOT(slotRGReady(QList<RGInfo>&)));
    }
}

void GPSReverseGeocodingWidget::updateUIState()
{
    const bool haveSelection = d->selectionModel->hasSelection();

    d->buttonRGSelected->setEnabled(d->UIEnabled && haveSelection);
    d->serviceLabel->setEnabled(d->UIEnabled);
    d->serviceComboBox->setEnabled(d->UIEnabled);
    d->languageLabel->setEnabled(d->UIEnabled);
    d->languageEdit->setEnabled(d->UIEnabled);

    d->baseTagLabel->setEnabled(d->UIEnabled);
    d->baseTagEdit->setEnabled(d->UIEnabled);

    d->buttonHideOptions->setEnabled(d->UIEnabled);
    d->autoTag->setEnabled(d->UIEnabled);
    d->metadataLabel->setEnabled(d->UIEnabled);
    d->iptc->setEnabled(d->UIEnabled);
    d->xmpLoc->setEnabled(d->UIEnabled);
    d->xmpKey->setEnabled(d->UIEnabled);
}

GPSReverseGeocodingWidget::~GPSReverseGeocodingWidget()
{
    delete d;
}

void GPSReverseGeocodingWidget::slotButtonRGSelected()
{
    // get the selected image:
    const QModelIndexList selectedItems = d->selectionModel->selectedRows();
    d->backendIndex = d->serviceComboBox->currentIndex(); 
    d->currentBackend = d->backendRGList[d->backendIndex];


    QList<RGInfo> photoList;

    QString wantedLanguage = d->languageEdit->itemData(d->languageEdit->currentIndex()).toString(); 

    for( int i = 0; i < selectedItems.count(); ++i)
    {

        const QPersistentModelIndex itemIndex = selectedItems.at(i);
        const GPSImageItem* const selectedItem = static_cast<GPSImageItem*>(d->imageModel->itemFromIndex(itemIndex));


        const GPSDataContainer gpsData = selectedItem->gpsData();
         if (!gpsData.hasCoordinates())
            return;

        const qreal latitude = gpsData.getCoordinates().lat();
        const qreal longitude = gpsData.getCoordinates().lon();

        RGInfo photoObj;
        photoObj.id = itemIndex;
        photoObj.coordinates = KMapIface::WMWGeoCoordinate(latitude, longitude);

        photoList << photoObj;
    }

    if (!photoList.isEmpty())
    {
        d->receivedRGCount = 0;
        d->requestedRGCount = photoList.count();
        emit(signalProgressSetup(d->requestedRGCount, i18n("Retrieving RG info - %p%")));
        emit(signalSetUIEnabled(false));

        d->currentBackend->callRGBackend(photoList, wantedLanguage);
    }
}

void GPSReverseGeocodingWidget::slotHideOptions()
{

    if(d->hideOptions)
    {
        d->LGridContainer->hide();
        d->hideOptions = false;
        d->buttonHideOptions->setText("More options");
    }
    else{
        d->LGridContainer->show();
        d->hideOptions = true;
        d->buttonHideOptions->setText("Less options");
    }

}

QStringList RemoveDuplicateTags(QStringList tagList)
{
    for(int i=0; i<tagList.count(); ++i)
    {
        for(int j=0; j<tagList.count(); ++j)
        {
            if(tagList[i].contains(tagList[j],Qt::CaseSensitive) && (i != j))
            {
                tagList.removeAt(j);
            }
        }
    }

    tagList.removeDuplicates();
    return tagList;
}

void GPSReverseGeocodingWidget::slotRGReady(QList<RGInfo>& returnedRGList)
{

    const QString errorString = d->currentBackend->getErrorMessage();
    
    if(!errorString.isEmpty())
    {

        KMessageBox::error(this, errorString);
        
        d->receivedRGCount+=returnedRGList.count();
        emit(signalSetUIEnabled(true));
        return;
    } 

    QString address;
    for(int i = 0; i < returnedRGList.count(); ++i)
    {

        QPersistentModelIndex currentImageIndex = returnedRGList[i].id;

        QString result = makeTagString(returnedRGList[i], d->baseTagEdit->displayText(), d->currentBackend->backendName());

        QString combinedResult = makeTagString(returnedRGList[i], "{Country}/{City}", d->currentBackend->backendName());

        //TODO: see what happens when no country nor city is returned
        int separatorIndex = combinedResult.indexOf(QString("%1").arg("/"));
        QString countryResult = combinedResult.left(separatorIndex);
        QString cityResult = combinedResult.mid(separatorIndex+1, combinedResult.length()-separatorIndex-1);

        QStringList elements, resultedData;
        elements<<QString("%1").arg("{Country}")<<QString("%1").arg("{City}");
        resultedData<<countryResult<<cityResult;

        QStringList returnedTags = d->tagModel->addNewData(elements, resultedData);   

        returnedTags = RemoveDuplicateTags(returnedTags);

        kDebug()<<"Returned tags:"<<returnedTags;

        TagData tagStructure;
        tagStructure.tags = returnedTags; 

        GPSImageItem* currentItem = static_cast<GPSImageItem*>(d->imageModel->itemFromIndex(currentImageIndex));
        currentItem->setTagInfo(result);
        currentItem->setTagData(tagStructure);
    }

    d->receivedRGCount+=returnedRGList.count();
    if (d->receivedRGCount>=d->requestedRGCount)
    {

        emit(signalSetUIEnabled(true));
    }
    else
    {
        emit(signalProgressChanged(d->receivedRGCount));
    }
} 

void GPSReverseGeocodingWidget::setUIEnabled(const bool state)
{
    d->UIEnabled = state;
    updateUIState();
}

void GPSReverseGeocodingWidget::treeItemClicked( const QModelIndex& index)
{
    
    kDebug()<<"Tag data:"<<d->tagModel->data(index, Qt::DisplayRole);

    //d->tagModel->addNewTags(index, "New Country");
    //d->tagModel->addSpacerTag(index, "New Country");

}

bool GPSReverseGeocodingWidget::eventFilter(QObject* watched, QEvent* event)
{
    if(watched == d->tagTreeView)
    {
        if((event->type()==QEvent::ContextMenu) && d->tagSelectionModel->hasSelection() )
        {
            
            KMenu * const menu = new KMenu(d->tagTreeView);
            menu->addAction(d->actionAddCountry);
            menu->addAction(d->actionAddCity);
            
            QContextMenuEvent * const e = static_cast<QContextMenuEvent*>(event);
            menu->exec(e->globalPos());

        }

    }

    return QObject::eventFilter(watched, event);

}

void GPSReverseGeocodingWidget::saveSettingsToGroup(KConfigGroup* const group)
{
    
    group->writeEntry("RG Backend", d->serviceComboBox->currentIndex()); 
    group->writeEntry("Language", d->languageEdit->currentIndex());

    group->writeEntry("Hide options", d->hideOptions); 
    group->writeEntry("IPTC", d->iptc->isChecked());
    group->writeEntry("XMP location", d->xmpLoc->isChecked());
    group->writeEntry("XMP keywords", d->xmpKey->isChecked());

}

void GPSReverseGeocodingWidget::readSettingsFromGroup(const KConfigGroup* const group)
{

    d->serviceComboBox->setCurrentIndex(group->readEntry("RG Backend", 0));
    d->languageEdit->setCurrentIndex(group->readEntry("Language", 0));

    d->hideOptions = !(group->readEntry("Hide options", false));
    slotHideOptions();


    d->iptc->setChecked(group->readEntry("IPTC", false));
    d->xmpLoc->setChecked(group->readEntry("XMP location", false));
    d->xmpKey->setChecked(group->readEntry("XMP keywords", false));
    
    

}

void GPSReverseGeocodingWidget::slotAddCountry()
{
    const QModelIndex baseIndex = d->tagSelectionModel->currentIndex();
    d->tagModel->addSpacerTag(baseIndex, "{Country}");

}

void GPSReverseGeocodingWidget::slotAddCity()
{
    const QModelIndex baseIndex = d->tagSelectionModel->currentIndex();
    d->tagModel->addSpacerTag(baseIndex, "{City}");
}

} /* KIPIGPSSyncPlugin  */


