#ifndef CTVIEWER_H
#define CTVIEWER_H

#include <QMainWindow>

#include <QtWidgets/QWidget>
#include <qmainwindow.h>
#include "ui_ctviewer.h"
#include <QString>

#include <vtkEventQtSlotConnect.h>
#include <vtkInteractorStyle.h>
#include <QVTKWidget.h>

#include <vtkActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkAxesActor.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkImageProperty.h>
#include <vtkImageSlice.h>
#include <vtkImageResliceMapper.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLine.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyData.h>
#include <vtkPlane.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkTextActor.h>
#include <vtkDICOMImageReader.h>

#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageToVTKImageFilter.h>
#include <itkGDCMImageIO.h>
#include <itkVTKImageToImageFilter.h>
#include <itkConnectedThresholdImageFilter.h>

#include <QFileDialog>
#include <QDir>
#include <qstring.h>

#include <vtkImageThreshold.h>

#include <vtkImageActor.h>
#include <vtkImageViewer2.h>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QColorDialog>
#include <vtkImageConstantPad.h>
#include <vtkPlaneSource.h>
#include <vtkLookupTable.h>
#include <vtkTexture.h>

#include <iostream>
#include <string>
#include <Qlist>
#include <iostream>  
#include <list>  
#include <algorithm>  
#include <string>  
#include <vector> 

#include <vtkImageMarchingCubes.h>
#include <vtkStripper.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkSTLWriter.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkOutlineFilter.h>
#include <vtkSurfaceReconstructionFilter.h>
#include <vtkContourFilter.h>
#include <vtkDelaunay2D.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkDataSetMapper.h>
#include <vtkNamedColors.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkImageOpenClose3D.h>

QT_BEGIN_NAMESPACE
namespace Ui { class CTViewer; }
QT_END_NAMESPACE

//volume render

#include <list>
using namespace std;

class SegItem;
class VolumeItem;

/* illustrate :
* 0 : Axial Viewer
* 1 : Sagittal Viewer
* 2 : Coronal Viewer
* 3 : 3D viewer
*/

class CTViewer : public QMainWindow
{
	Q_OBJECT

	typedef itk::Image<signed short, 3> ITKImage3DType_SIGNEDSHORT;
	typedef ITKImage3DType_SIGNEDSHORT ITKImage3DType;

	typedef short pixelType;
	static const unsigned int nDimension = 3;
	typedef itk::Image<pixelType, nDimension> Image3DType;
	typedef itk::ConnectedThresholdImageFilter<Image3DType, Image3DType> ConnectedFilterType;
	typedef itk::ImageToVTKImageFilter<Image3DType> ConnectorType;


public:
	CTViewer(QMainWindow* parent = 0);
	~CTViewer();

	vtkRenderer* GetRenderer(int i);
	vtkRenderer* GetNewLayerRenderer(int i);
	vtkRenderWindow* GetRenderWindow(int i);
	vtkPlane* GetPlane(int i);
	vtkImageData* GetImageData();
	void GetVolumeRenderBound(double* bound);

	void SetWindowText(int i, QString tex);
	void SetVolumeRenderParameter(double* min, double* max, double* opacity, double* bound);
	void SetVolumeRender(bool* flag);//cancel volume render for efficient operate the execution

	void SetPickFiducialPoints();

	vtkSmartPointer<vtkImageData> CTViewer::ThresholdSegment(vtkImageData* data, double lower, double upper);
	//void OnSetSlicePosition(double x, double y, double z);
	//MainWindow
	void onAddSegItem(int row, QColor color);
	void onAddVolumeItem(int row, QColor color);
	void InputImageDisplay(vtkImageData* image, int lower, int higher);
	
	int m_thresMax, m_thresMin;
	vtkSmartPointer<vtkImageData> thresholded;
	//void InputImageDisplay();
	vtkImageSlice* InImageSlice[3];

	//RG
	bool CheckSeedPointInsideImage(double worldSeedPoint[3]);

signals:
	void sigRightButtonPress(double, double, double);
	void sigSliceChanged();

private:
	Ui::CTViewer ui;

	itk::GDCMImageIO::Pointer GDCMIO;

	double m_ZoomFactor;
	double m_LineCenter[3];
	double m_Spacing[3], m_Origin[3];
	int m_Extend[6];
	int m_Window, m_Level, m_VolumeMin, m_VolumeMax;
	double m_Opacity;

	double m_MiddlePoint[3];
	double m_Bound[6];

	vtkImageData* m_ImageData;
	vtkImageData* m_vtkImageData;
	//vtkSmartPointer<vtkImageData> thresholded;
	//render
	QVTKWidget* m_QVTKWidget[4];
	/*
	vtkRenderer* m_Renderer[4];
	vtkRenderWindow* m_RenderWindow[4];
	vtkRenderWindowInteractor* m_Interactor[4];
	vtkInteractorStyle* m_2DInteractorStyle[3];
	vtkRenderer* m_Renderer1[4];
	vtkInteractorStyleTrackballCamera* m_3DInteractorStyle;
	*/
	vtkSmartPointer <vtkRenderer> m_Renderer[4];
	vtkSmartPointer <vtkRenderWindow> m_RenderWindow[4];
	vtkRenderWindowInteractor* m_Interactor[4];
	vtkInteractorStyle* m_2DInteractorStyle[3];
	vtkSmartPointer <vtkRenderer> m_Renderer1[3];
	vtkInteractorStyleTrackballCamera* m_3DInteractorStyle;

	vtkEventQtSlotConnect* m_EventQtSlotConnect;
	//MainWindow
	vtkSmartPointer<vtkLookupTable> m_thresholdLookupTable;

	QVTKWidget* mw_QVTKWidget;
	vtkSmartPointer <vtkRenderer> mw_Renderer;
	vtkSmartPointer <vtkRenderWindow> mw_RenderWindow;
	vtkSmartPointer <vtkRenderWindowInteractor> mw_Interactor;
	vtkSmartPointer <vtkInteractorStyleTrackballCamera> m_Style;
	QList<SegItem*> m_SegItemList;
	QList<VolumeItem*> m_VolumeItemList;
	CTViewer* m_CTViewer;
	vtkSmartPointer<vtkImageViewer2> imageViewer[3];
	
	vtkSmartPointer<vtkImageThreshold> m_thresholdFilter;
	//二维切片
	vtkImageProperty* m_ImageSliceProperty;
	vtkPlane* m_Plane[3];
	vtkImageResliceMapper* m_ResliceMapper[3];
	vtkImageResliceMapper* m_ResliceMapper3D[3];
	vtkImageSlice* m_ImageSlice[3];
	vtkImageSlice* m_ImageSlice3D[3];


	//表示层的三维坐标
	vtkLine* m_SliceLine[6];
	vtkPolyData* m_SliceLineData[6];
	vtkActor* m_SliceLineActor[6];

	//三维视图中坐标widget
	vtkAxesActor* m_3DAxesActor;
	vtkAnnotatedCubeActor* m_3DCubeActor;
	vtkOrientationMarkerWidget* m_3DOrientationWidget;

	//volume render
	vtkSmartVolumeMapper* m_VolumeMapper;
	vtkVolumeProperty* m_VolumeProperty;
	vtkPiecewiseFunction* m_CompositeOpacity;
	vtkVolume* m_Volume;
	vtkColorTransferFunction* m_ColorTrans;

	void CreatAction();
	void InitializeViewer();

	void SetWindowText(vtkRenderer* ren, QString text, double* color);
	void SetWindowText(vtkRenderer* ren, vtkTextActor* actor, QString text, double* color, double coord1, double coord2);

	void SetViewerParamters();
	void SetSingleImageReslice(int mark);
	void SetCoodinateLines();

	void VolumeRender();

	void ShowUpLeftView();
	void ShowUpRightView();
	void ShowLowerLeftView();
	void ShowLowerRightView();
	void ShowNormal();

	//RG
	Image3DType::Pointer __mItkImage;
	Image3DType::IndexType __mPixelIndex;
	vtkImageData* __originalImageData;
	vtkImageData* __outputImageData;
	ConnectedFilterType::Pointer __connectedThreshold;
	ConnectorType::Pointer __connector;
	int __lowerThreshold;
	int __upperThreshold;
	int __replaceValue;

public slots:
	void OnSetImageData(vtkImageData* data);
	void OnSetMeshFile(QString filepath);

	void OnResetCamera();
	void OnSetZoomIn();
	void OnSetZoomOut();
	void OnSetCoordinateLinesVisible(bool);
	void OnSet3DCoordinateLinesVisible(bool);
	void OnSetSlicePosition(double, double, double);
	void OnSetFicalPointAndZoom(double x, double y, double z, double zoom);
	void OnSetRender3FocusPointAndOrientation(double x, double y, double z, double rx, double ry, double rz);

	//void OnShowVolume(int state);
	//void OnShow3DSlice(int state);
	//MainWindow
	void onThresholdApply();
	void onSegItemColorChange();
	void onThresholdDelete();
	void onSegItemVisable(int state);
	void onSegItemLowHigh();
	void OnLimitChange();
	void onPreview(int state);
	void onDisable(int state);
	void OnOpenDicomFile();
	void OnRG();
	void OnReconstruction();
	void onVolumeItemVisable(int state);
	void onVolumeColorChange();
	void onSetVolumeOpacity(int value);
	void onExportSTL();

private slots:
	void OnSetWindowLevel();
	void OnUpdataViews();
	void OnUpdataLowerRightView();

	//mouse interactor
	void OnMiddleButton(vtkObject* caller, unsigned long vtk_event);
	void OnLeftButtonPress(vtkObject* caller, unsigned long vtk_event);
	void OnRightButtonPress(vtkObject* caller, unsigned long vtk_event);

	//layout
	void OnUpLeftLayout(bool);
	void OnUpRightLayouy(bool);
	void OnLowerLeftLayout(bool);
	void OnLowerRightLayout(bool);
};

//左侧选项区

class SegItem
{	//Q_OBJECT

public:
	SegItem(CTViewer* CTViewer, vtkImageData* data, int lower, int higher);
	~SegItem();

	QColor GetSegItemColor();
	void OnSetColor(QColor color);
	void OnSetLimit(int lower, int higher);
	void OnSetLowerEdit(QLineEdit* EditLower);
	void OnSetHigherEdit(QLineEdit* EditHigher);
	vtkSmartPointer<vtkImageData>
		ThresholdSegment(vtkImageData* data, double lower, double higher);
	void InputImageDisplay();
	vtkSmartPointer<vtkImageData> GetSegItemData();
	vtkImageSlice* GetInImageSlice(int i);
	QLineEdit* OnGetLowerEdit();
	QLineEdit* OnGetHigherEdit();


	vtkImageData* m_InputImageData;
	vtkSmartPointer<vtkImageData> m_SegItemData;
	int m_RdColor[3];
	QColor m_Color;
	int m_lower;
	int m_higher;
	vtkImageSlice* InImageSlice[3];
	CTViewer* m_CTViewer;
	vtkSmartPointer<vtkLookupTable> m_ColorTable;
	QLineEdit* m_EditLower;
	QLineEdit* m_EditHigher;
	vtkSmartPointer<vtkImageThreshold> thresholdFilter;



private:

};

class VolumeItem
{	//Q_OBJECT

public:
	VolumeItem(CTViewer* CTViewer1, vtkImageData* ddata, QColor color);
	~VolumeItem();
	//vtkImageData* m_VolumeImageData;
	vtkSmartPointer<vtkImageData> m_VolumeImageData;
	CTViewer* m_CTViewer1;
	QColor m_Color1;
	double m_Opacity1;
	double m_VolumeMin1;
	double m_VolumeMax1;
	double m_level1;
	QSlider* m_slider;
	vtkProperty* m_VolumeProperty1;
	vtkPiecewiseFunction* m_CompositeOpacity1;
	vtkColorTransferFunction* m_ColorTrans1;
	//vtkVolume* m_Volume1;
	vtkActor* m_Volume1;
	vtkSmoothPolyDataFilter* m_Filter1;

	void ReconstructVolume();
	//vtkVolume* GetVolume();
	vtkActor* GetVolume();
	vtkSmoothPolyDataFilter* GetFilter();

	void OnSetColor(QColor color);
	void OnSetOpacity(double opacity);
	void OnSetSlider(QSlider* slider);
	QSlider* OnGetSlider();
	QColor GetVolItemColor();

	//QColor GetSegItemColor();
	//
	//void OnSetLimit(int lower, int higher);
	//void OnSetLowerEdit(QLineEdit* EditLower);
	//void OnSetHigherEdit(QLineEdit* EditHigher);
	//vtkSmartPointer<vtkImageData>
	//	ThresholdSegment(vtkImageData* data, double lower, double higher);
	//void InputImageDisplay();
	//vtkSmartPointer<vtkImageData> GetSegItemData();
	//vtkImageSlice* GetInImageSlice(int i);
	//QLineEdit* OnGetLowerEdit();
	//QLineEdit* OnGetHigherEdit();


	//
	//int m_lower;
	//int m_higher;
	//vtkImageSlice* InImageSlice[3];
	//
	//vtkSmartPointer<vtkLookupTable> m_ColorTable;
	//QLineEdit* m_EditLower;
	//QLineEdit* m_EditHigher;
	//vtkSmartPointer<vtkImageThreshold> thresholdFilter;



private:

};



#endif // CTVIEWER_H
