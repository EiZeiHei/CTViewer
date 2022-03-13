#include "ctviewer.h"
#include "./ui_ctviewer.h"

#include <math.h>
#include <qmath.h>
#include <vtkFloatArray.h>
#include <vtkPieChartActor.h>
#include <vtkDataObject.h>
#include <vtkFloatArray.h>
#include <vtkAbstractPicker.h>
#include <vtkSphereWidget.h>
#include <vtkProperty.h>
#include <vtkPropAssembly.h>
#include <vtkSmartPointer.h>
#include <vtkActor2DCollection.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkCamera.h>

#include <vtkCellArray.h>
#include <vtkPolyDataMapper.h>
#include <vtkCellPicker.h>
#include <vtkMath.h>
#include <vtkSTLReader.h>
#include <vtkTriangleFilter.h>
#include <vtkRendererCollection.h>

#include <QVTKWidget.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkLookupTable.h>
#include <vtkImageMapToColors.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <vtkDICOMImageReader.h>
#include <qcolordialog.h>
#include <qcolor.h>

#include "time.h"
#include <random>

//bu
#include "vtkAutoInit.h" 
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

//************************************
// Method:    RandomCreatFunc
// FullName:  RandomCreatFunc
// Access:    public 
// Returns:   int
// Qualifier: 根据区间生成随机数，随机数范围：[interval_min,interval_max]
// Parameter: int interval_min 区间左值
// Parameter: int interval_max 区间右值
//************************************
int RandomCreatFunc(int interval_min, int interval_max)
{
	if (interval_min >= interval_max)
		return INT_MAX;
	//种子值是通过 random_device 类型的函数对象 rd 获得的。
	//每一个 rd() 调用都会返回不同的值，而且如果我们实现的 random_devic 是非确定性的，程序每次执行连续调用 rd() 都会产生不同的序列。
	random_device rd;
	default_random_engine e{ rd() };
	int random_num = 0;
	//分情况产生随机数，考虑3种情况：min<0&&max>0 、max<0和min>0
	if (interval_min >= 0)//min>0的情况
	{
		uniform_int_distribution<unsigned> a(interval_min, interval_max);
		random_num = a(e);
	}
	else if (interval_max > 0)//min<0&&max>0 的情况
	{
		uniform_int_distribution<unsigned> a(0, -interval_min);
		random_num = a(e);
		random_num = -random_num;
		uniform_int_distribution<unsigned> b(0, 10);
		if (b(e) % 2 == 0)
		{
			uniform_int_distribution<unsigned> c(0, interval_max);
			random_num = c(e);
		}
	}
	else//max<0的情况
	{
		uniform_int_distribution<unsigned> a(-interval_max, -interval_min);
		random_num = a(e);
		random_num = -random_num;
	}
	return random_num;

}

CTViewer::CTViewer(QMainWindow *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);
	

	m_ImageData = nullptr;
	m_Window = 1000;
	m_Level = 500;
	m_Opacity = 1;
	m_VolumeMin = -500;
	m_VolumeMax = 1500;

	m_ZoomFactor = 0.5;

	this->InitializeViewer();
	this->CreatAction();

}

CTViewer::~CTViewer()
{
	for (int i = 0; i < 4; ++i)
	{
		m_QVTKWidget[i] = nullptr;
	}
	for (int i = 0; i < 6; ++i)
	{
		//表示层的三维坐标
		m_SliceLine[i]->Delete();
		m_SliceLineData[i]->Delete();
		m_SliceLineActor[i]->Delete();
	}
	for (int i = 0; i < 3; ++i)
	{
		m_Renderer1[i]->Delete();
		m_2DInteractorStyle[i]->Delete();
		m_Plane[i]->Delete();
		m_ResliceMapper[i]->Delete();
		m_ResliceMapper3D[i]->Delete();
		m_ImageSlice[i]->Delete();
		m_ImageSlice3D[i]->Delete();

		m_Renderer1[i] = nullptr;
		m_2DInteractorStyle[i] = nullptr;
		m_Plane[i] = nullptr;
		m_ResliceMapper[i] = nullptr;
		m_ResliceMapper3D[i] = nullptr;
		m_ImageSlice[i] = nullptr;
		m_ImageSlice3D[i] = nullptr;

		m_SliceLine[i] = nullptr;
		m_SliceLineData[i] = nullptr;
		m_SliceLineActor[i] = nullptr;
	}
	for (int i = 0; i < 4; ++i)
	{
		m_Renderer[i]->Delete();
		m_RenderWindow[i]->Delete();
		m_Interactor[i]->Delete();

		m_Interactor[i] = nullptr;
		m_Renderer[i] = nullptr;
		m_RenderWindow[i] = nullptr;
	}

	m_VolumeMapper->Delete();
	m_VolumeProperty->Delete();
	m_CompositeOpacity->Delete();
	m_Volume->Delete();
	m_ColorTrans->Delete();

	m_EventQtSlotConnect->Delete();
	m_3DInteractorStyle->Delete();
	m_ImageSliceProperty->Delete();

	m_EventQtSlotConnect = nullptr;
	m_3DInteractorStyle = nullptr;
	m_ImageSliceProperty = nullptr;

	m_VolumeMapper = nullptr;
	m_VolumeProperty = nullptr;
	m_CompositeOpacity = nullptr;
	m_Volume = nullptr;
	m_ColorTrans = nullptr;
}

//public function
vtkRenderer* CTViewer::GetRenderer(int i)
{
	return m_Renderer[i];
}

vtkRenderer* CTViewer::GetNewLayerRenderer(int i)
{
	return m_Renderer1[i];
}

vtkRenderWindow* CTViewer::GetRenderWindow(int i)
{
	return m_RenderWindow[i];
}

vtkPlane* CTViewer::GetPlane(int i)
{
	return m_Plane[i];
}

vtkImageData* CTViewer::GetImageData()
{
	return m_ImageData;
}

void CTViewer::GetVolumeRenderBound(double* bound)
{
	if (m_VolumeMapper == nullptr && bound == nullptr)
	{
		return;
	}

	for (int i = 0; i < 6; i++)
	{
		bound[i] = m_VolumeMapper->GetCroppingRegionPlanes()[i];
	}
}

//private function
void CTViewer::CreatAction()
{
	this->connect(ui.AxialSlider, SIGNAL(valueChanged(int)), this, SLOT(OnUpdataViews()));
	this->connect(ui.SagittalSlider, SIGNAL(valueChanged(int)), this, SLOT(OnUpdataViews()));
	this->connect(ui.CoronalSlider, SIGNAL(valueChanged(int)), this, SLOT(OnUpdataViews()));
	//this->connect(ui.ModelSlider, SIGNAL(valueChanged(int)), this, SLOT(OnUpdataModelView()));

	this->connect(ui.windowScrollBar, SIGNAL(valueChanged(int)), this, SLOT(OnSetWindowLevel()));
	this->connect(ui.levelScrollBar, SIGNAL(valueChanged(int)), this, SLOT(OnSetWindowLevel()));

	/*	
	this->connect(ui.escAxialButton,SIGNAL(clicked(bool)),this,SLOT(OnAxialLayout(bool)));
	this->connect(ui.escSagittalButton,SIGNAL(clicked(bool)),this,SLOT(OnSagittalLayouy(bool)));
	this->connect(ui.escCoronalButton,SIGNAL(clicked(bool)),this,SLOT(OnCoronalLayout(bool)));
	this->connect(ui.escModelbutton,SIGNAL(clicked(bool)),this,SLOT(OnModelLayout(bool)));
	*/

	/*********************************************************************************************/
	for (int i = 0; i < 3; ++i)
	{
		m_EventQtSlotConnect->Connect(m_Interactor[i], vtkCommand::MouseWheelForwardEvent,
			this, SLOT(OnMiddleButton(vtkObject*, unsigned long)));
		m_EventQtSlotConnect->Connect(m_Interactor[i], vtkCommand::MouseWheelBackwardEvent,
			this, SLOT(OnMiddleButton(vtkObject*, unsigned long)));
		m_EventQtSlotConnect->Connect(m_Interactor[i], vtkCommand::LeftButtonPressEvent,
			this, SLOT(OnLeftButtonPress(vtkObject*, unsigned long)));
	}

	this->connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(OnOpenDicomFile()));
	this->connect(ui.threshapply, SIGNAL(clicked()), this, SLOT(onThresholdApply()));
	this->connect(ui.threshdelete, SIGNAL(clicked()), this, SLOT(onThresholdDelete()));
	this->connect(ui.threshMinSlider, SIGNAL(valueChanged(int)), this, SLOT(OnLimitChange()));
	this->connect(ui.threshMinSlider, SIGNAL(valueChanged(int)), ui.threshMinBox, SLOT(setValue(int)));
	this->connect(ui.threshMinBox, SIGNAL(valueChanged(int)), ui.threshMinSlider, SLOT(setValue(int)));
	this->connect(ui.threshMaxSlider, SIGNAL(valueChanged(int)), this, SLOT(OnLimitChange()));
	this->connect(ui.threshMaxSlider, SIGNAL(valueChanged(int)), ui.threshMaxBox, SLOT(setValue(int)));
	this->connect(ui.threshMaxBox, SIGNAL(valueChanged(int)), ui.threshMaxSlider, SLOT(setValue(int)));
	this->connect(ui.Preview, SIGNAL(stateChanged(int)), this, SLOT(onPreview(int)));
	this->connect(ui.Disable, SIGNAL(stateChanged(int)), this, SLOT(onDisable(int)));

	this->connect(ui.RG, SIGNAL(clicked()), this, SLOT(OnRG()));
	this->connect(ui.construct, SIGNAL(clicked()), this, SLOT(OnReconstruction()));

	ui.tableWidget->resizeColumnsToContents();//自适应列宽
	ui.tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.VolumeTableWidget->resizeColumnsToContents();//自适应列宽
	ui.VolumeTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	
}

void CTViewer::InitializeViewer()
{
	m_QVTKWidget[0] = ui.AxialWidget;
	m_QVTKWidget[1] = ui.SagittalWidget;
	m_QVTKWidget[2] = ui.CoronalWidget;
	m_QVTKWidget[3] = ui.ModelWidget;


	for (int i = 0; i < 3; i++)
	{
		m_Renderer[i] = vtkRenderer::New();
		m_Renderer1[i] = vtkRenderer::New();
		m_RenderWindow[i] = vtkRenderWindow::New();
		m_Interactor[i] = vtkRenderWindowInteractor::New();
		m_2DInteractorStyle[i] = vtkInteractorStyle::New();

		m_Renderer[i]->SetLayer(0);
		m_Renderer1[i]->SetLayer(1);
		m_Renderer[i]->SetActiveCamera(m_Renderer1[i]->GetActiveCamera());

		m_RenderWindow[i]->AddRenderer(m_Renderer[i]);
		m_RenderWindow[i]->AddRenderer(m_Renderer1[i]);
		m_RenderWindow[i]->SetNumberOfLayers(2);
		m_Interactor[i]->SetInteractorStyle(m_2DInteractorStyle[i]);
		m_RenderWindow[i]->SetInteractor(m_Interactor[i]);

		//三个二维切面
		m_Plane[i] = vtkPlane::New();
		m_ResliceMapper[i] = vtkImageResliceMapper::New();
		m_ResliceMapper3D[i] = vtkImageResliceMapper::New();
		m_ImageSlice[i] = vtkImageSlice::New();
		m_ImageSlice3D[i] = vtkImageSlice::New();
	}

	for (int i = 0; i < 6; i++)
	{
		m_SliceLine[i] = vtkLine::New();
		m_SliceLineData[i] = vtkPolyData::New();
		m_SliceLineActor[i] = vtkActor::New();
	}

	m_EventQtSlotConnect = vtkEventQtSlotConnect::New();
	m_ImageSliceProperty = vtkImageProperty::New();

	m_SliceLineActor[0]->GetProperty()->SetColor(1, 0, 0);
	m_SliceLineActor[1]->GetProperty()->SetColor(0, 1, 0);
	m_SliceLineActor[2]->GetProperty()->SetColor(0, 0, 1);
	m_SliceLineActor[3]->GetProperty()->SetColor(1, 0, 0);
	m_SliceLineActor[4]->GetProperty()->SetColor(0, 1, 0);
	m_SliceLineActor[5]->GetProperty()->SetColor(0, 0, 1);

	//axial
	m_Plane[0]->SetOrigin(0, 0, 0);
	m_Plane[0]->SetNormal(0, 0, 1);
	//sagittal
	m_Plane[1]->SetOrigin(0, 0, 0);
	m_Plane[1]->SetNormal(1, 0, 0);
	//coronal
	m_Plane[2]->SetOrigin(0, 0, 0);
	m_Plane[2]->SetNormal(0, 1, 0);

	//配置第四个视图
	m_Renderer[3] = vtkRenderer::New();
	m_RenderWindow[3] = vtkRenderWindow::New();
	m_3DInteractorStyle = vtkInteractorStyleTrackballCamera::New();
	m_Interactor[3] = vtkRenderWindowInteractor::New();
	m_RenderWindow[3]->AddRenderer(m_Renderer[3]);
	m_Interactor[3]->SetInteractorStyle(m_3DInteractorStyle);
	m_RenderWindow[3]->SetInteractor(m_Interactor[3]);
	m_Renderer[3]->SetBackground(0, 0, 0);  // 设置页面底部颜色值
	//m_Renderer[3]->SetBackground2(0.529, 0.8078, 0.92157);    // 设置页面顶部颜色值
	//m_Renderer[3]->SetGradientBackground(1);//渐变色开启

	//设置视图名称
	double textcolor[4][3] = { 1,0,0,0,1,0,0,0,1,0.5,0,0.5 };
	QString text[4] = { tr("Axial"),tr("Sagittal"),tr("Coronal"),tr("Model") };
	for (int i = 0; i < 4; i++)
	{
		//	m_Viewer[i]->SetupInteractor(m_Interactor[i]);
		m_QVTKWidget[i]->SetRenderWindow(m_RenderWindow[i]);
		m_Interactor[i]->Initialize();
		this->SetWindowText(m_Renderer[i], text[i], textcolor[i]);
	}

	//volume render
	m_VolumeMapper = vtkSmartVolumeMapper::New();
	m_VolumeProperty = vtkVolumeProperty::New();
	m_CompositeOpacity = vtkPiecewiseFunction::New();//分段线性函数
	m_ColorTrans = vtkColorTransferFunction::New();//将一个标量值映射为颜色值
	m_Volume = vtkVolume::New();

	//设置方向widget
	m_3DAxesActor = vtkAxesActor::New();
	m_3DCubeActor = vtkAnnotatedCubeActor::New();

	auto probAssemble = vtkPropAssembly::New();
	probAssemble->AddPart(m_3DAxesActor);
	probAssemble->AddPart(m_3DCubeActor);

	m_3DOrientationWidget = vtkOrientationMarkerWidget::New();
	m_3DOrientationWidget->SetOrientationMarker(probAssemble);
	m_3DOrientationWidget->SetInteractor(m_Interactor[3]);
	m_3DOrientationWidget->SetViewport(0.85, 0, 1, 0.2);
	m_3DOrientationWidget->SetEnabled(1);
	m_3DOrientationWidget->InteractiveOn();

	probAssemble->Delete(); probAssemble = nullptr;
}

//文字层设置
void CTViewer::SetWindowText(vtkRenderer* ren, QString text, double* color)
{
	//删除视图当前标识
	auto propCollection = vtkSmartPointer<vtkActor2DCollection>::New();
	propCollection = ren->GetActors2D();
	propCollection->InitTraversal();
	for (int i = 0; i < propCollection->GetNumberOfItems(); i++)
	{
		ren->RemoveActor2D(propCollection->GetNextProp());
	}

	auto textActor = vtkSmartPointer<vtkTextActor>::New();
	textActor->SetInput(qPrintable(text));
	textActor->SetTextScaleModeToViewport();
	textActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
	textActor->GetPositionCoordinate()->SetValue(0.05, 0.07);
	textActor->GetTextProperty()->SetFontFamilyToArial();
	textActor->GetTextProperty()->SetFontSize(24);
	textActor->GetTextProperty()->BoldOn();
	textActor->GetTextProperty()->ShadowOff();
	textActor->GetTextProperty()->ItalicOff();
	textActor->GetTextProperty()->SetColor(color);
	ren->AddActor2D(textActor);//添加新的标识
	ren->Render();
}

void CTViewer::SetWindowText(int k, QString text)
{
	//删除视图当前标识
	auto propCollection = vtkSmartPointer<vtkActor2DCollection>::New();
	propCollection = m_Renderer[k]->GetActors2D();
	propCollection->InitTraversal();
	for (int i = 0; i < propCollection->GetNumberOfItems(); i++)
	{
		m_Renderer[k]->RemoveActor2D(propCollection->GetNextProp());
	}

	auto textActor = vtkSmartPointer<vtkTextActor>::New();
	textActor->SetInput(qPrintable(text));
	textActor->SetTextScaleModeToViewport();
	textActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
	textActor->GetPositionCoordinate()->SetValue(0.05, 0.07);
	textActor->GetTextProperty()->SetFontFamilyToArial();
	textActor->GetTextProperty()->SetFontSize(15);
	textActor->GetTextProperty()->BoldOn();
	textActor->GetTextProperty()->ShadowOff();
	textActor->GetTextProperty()->ItalicOff();
	textActor->GetTextProperty()->SetColor(1, 0, 0);
	m_Renderer[k]->AddActor2D(textActor);//添加新的标识
	m_Renderer[k]->Render();
}

void CTViewer::SetVolumeRenderParameter(double* min, double* max, double* opacity, double* bound)
{
	if (!m_Volume)
	{
		return;
	}
	m_VolumeMin = *min;
	m_VolumeMax = *max;
	double level = (*min + *max) * 0.5;
	m_Opacity = *opacity;

	m_CompositeOpacity->RemoveAllPoints();
	m_CompositeOpacity->AddPoint(*min, 1);
	m_CompositeOpacity->AddPoint(*min + 1, 0.5 * m_Opacity);
	m_CompositeOpacity->AddPoint(level, 0.7 * m_Opacity);
	m_CompositeOpacity->AddPoint(*max - 1, 0.8 * m_Opacity);
	m_CompositeOpacity->AddPoint(*max, 1 * m_Opacity);

	m_ColorTrans->RemoveAllPoints();
	m_ColorTrans->AddRGBPoint(*min, 0.8, 0.8, 0.8);
	m_ColorTrans->AddRGBPoint(level, 0.8, 0.8, 0.8);
	m_ColorTrans->AddRGBPoint(*max, 1.00, 1.00, 1.00);
	m_VolumeProperty->SetColor(m_ColorTrans);//设置颜色传输函数

	double* scope = new double[6];
	for (int i = 0; i < 6; i++)
	{
		scope[i] = bound[i];
	}
	m_VolumeMapper->SetCroppingRegionPlanes(scope);

	m_RenderWindow[3]->Render();

	delete scope; scope = nullptr;
}

void CTViewer::SetVolumeRender(bool* flag)
{
	if (*flag)
	{
		m_Renderer[3]->AddVolume(m_Volume);
	}
	else
	{
		m_Renderer[3]->RemoveVolume(m_Volume);
	}
	m_Renderer[3]->GetRenderWindow()->Render();
}

void CTViewer::SetPickFiducialPoints()
{
	m_EventQtSlotConnect->Connect(m_Interactor[3], vtkCommand::RightButtonPressEvent,
		this, SLOT(OnRightButtonPress(vtkObject*, unsigned long)));
}

//二维图像初始化
void CTViewer::SetViewerParamters()
{
	//window and level
	ui.windowScrollBar->setMaximum(5000);
	ui.windowScrollBar->setMinimum(-1024);
	//ui.windowBox->setMaximum(5000);
	//ui.windowBox->setMinimum(-1024);
	ui.windowScrollBar->setValue(m_Window);
	ui.levelScrollBar->setMaximum(5000);
	ui.levelScrollBar->setMinimum(-1024);
	//ui.levelBox->setMaximum(5000);
	//ui.levelBox->setMinimum(-1024);
	ui.levelScrollBar->setValue(m_Level);

	//设置axial 视图
	int axial_middle = (m_Extend[5] - m_Extend[4]) / 2;
	ui.AxialSlider->setMaximum(m_Extend[5] - m_Extend[4]);
	ui.AxialSlider->setValue(axial_middle);

	//设置coronal视图
	int coronal_middle = (m_Extend[3] - m_Extend[2]) / 2;
	ui.CoronalSlider->setMaximum(m_Extend[3] - m_Extend[2]);
	ui.CoronalSlider->setValue(coronal_middle);

	//设置sagital视图
	int sagital_middle = (m_Extend[1] - m_Extend[0]) / 2;
	ui.SagittalSlider->setMaximum(m_Extend[1] - m_Extend[0]);
	ui.SagittalSlider->setValue(sagital_middle);
	
	//获取六个极限位置
	m_MiddlePoint[0] = (m_Bound[0] + m_Bound[1]) * 0.5;
	m_MiddlePoint[1] = (m_Bound[2] + m_Bound[3]) * 0.5;
	m_MiddlePoint[2] = (m_Bound[4] + m_Bound[5]) * 0.5;
	
	this->SetCoodinateLines();

	for (int i = 0; i < 3; ++i)
	{
		this->SetSingleImageReslice(i);
	}

	for (int i = 0; i < 6; ++i)
	{
		m_SliceLineActor[i]->GetProperty()->SetLineWidth(2);
		m_SliceLineActor[i]->SetPickable(0);
	}
	
	m_Renderer1[0]->RemoveActor(m_SliceLineActor[1]);
	m_Renderer1[0]->RemoveActor(m_SliceLineActor[5]);
	m_Renderer1[1]->RemoveActor(m_SliceLineActor[2]);
	m_Renderer1[1]->RemoveActor(m_SliceLineActor[3]);
	m_Renderer1[2]->RemoveActor(m_SliceLineActor[0]);
	m_Renderer1[2]->RemoveActor(m_SliceLineActor[4]);

	m_Renderer1[0]->AddActor(m_SliceLineActor[1]);
	m_Renderer1[0]->AddActor(m_SliceLineActor[5]);
	m_Renderer1[1]->AddActor(m_SliceLineActor[2]);
	m_Renderer1[1]->AddActor(m_SliceLineActor[3]);
	m_Renderer1[2]->AddActor(m_SliceLineActor[0]);
	m_Renderer1[2]->AddActor(m_SliceLineActor[4]);
	
	for (int i = 0; i < 4; i++)
	{
		m_RenderWindow[i]->Render();
	}
}

//切片显示
void CTViewer::SetSingleImageReslice(int i)
{
	m_ImageSliceProperty->SetColorWindow(m_Window);
	m_ImageSliceProperty->SetColorLevel(m_Level);
	m_ImageSliceProperty->SetInterpolationTypeToLinear();

	m_ResliceMapper[i]->SetInputData(m_ImageData);
	m_ResliceMapper[i]->SetSlicePlane(m_Plane[i]);
	m_ResliceMapper3D[i]->SetInputData(m_ImageData);
	m_ResliceMapper3D[i]->SetSlicePlane(m_Plane[i]);

	m_ImageSlice[i]->SetMapper(m_ResliceMapper[i]);
	m_ImageSlice[i]->SetProperty(m_ImageSliceProperty);
	m_ImageSlice3D[i]->SetMapper(m_ResliceMapper3D[i]);
	m_ImageSlice3D[i]->SetProperty(m_ImageSliceProperty);

	m_Renderer[i]->AddViewProp(m_ImageSlice[i]);

	switch (i)
	{
	case 0:
		m_Renderer[i]->GetActiveCamera()->SetViewUp(0, 1, 0);
		m_Renderer[i]->GetActiveCamera()->SetFocalPoint(m_LineCenter[0], m_LineCenter[1], m_LineCenter[2]);
		m_Renderer[i]->GetActiveCamera()->SetPosition(m_LineCenter[0], m_LineCenter[1], m_LineCenter[2] + 300);
		break;
	case 1:
		m_Renderer[i]->GetActiveCamera()->SetViewUp(0, 0, 1);
		m_Renderer[i]->GetActiveCamera()->SetFocalPoint(m_LineCenter[0], m_LineCenter[1], m_LineCenter[2]);
		m_Renderer[i]->GetActiveCamera()->SetPosition(m_LineCenter[0] + 300, m_LineCenter[1], m_LineCenter[2]);
		break;
	case 2:
		m_Renderer[i]->GetActiveCamera()->SetViewUp(0, 0, 1);
		m_Renderer[i]->GetActiveCamera()->SetFocalPoint(m_LineCenter[0], m_LineCenter[1], m_LineCenter[2]);
		m_Renderer[i]->GetActiveCamera()->SetPosition(m_LineCenter[0], m_LineCenter[1] - 300, m_LineCenter[2]);
		break;
	default:
		break;
	}

	m_RenderWindow[i]->Render();
}
//十字线显示
void CTViewer::SetCoodinateLines()
{
	m_LineCenter[0] = m_Spacing[0] * ui.SagittalSlider->value() + m_Origin[0];
	m_LineCenter[1] = m_Spacing[1] * ui.CoronalSlider->value() + m_Origin[1];
	m_LineCenter[2] = m_Spacing[2] * ui.AxialSlider->value() + m_Origin[2];

	for (int i = 0; i < 3; i++)
	{
		m_Plane[i]->SetOrigin(m_LineCenter);
	}
	
	//定义线的端点
	double xPoint1[3], xPoint2[3];
	double yPoint1[3], yPoint2[3];
	double zPoint1[3], zPoint2[3];

	//x 不变
	xPoint1[0] = m_Spacing[0] * m_Extend[0] + m_Origin[0];
	xPoint2[0] = m_Spacing[0] * m_Extend[1] + m_Origin[0];

	xPoint1[1] = m_LineCenter[1];
	xPoint2[1] = m_LineCenter[1];
	xPoint1[2] = m_LineCenter[2];
	xPoint2[2] = m_LineCenter[2];

	//y 不变
	yPoint1[1] = m_Spacing[1] * m_Extend[2] + m_Origin[1];
	yPoint2[1] = m_Spacing[1] * m_Extend[3] + m_Origin[1];

	yPoint1[0] = m_LineCenter[0];
	yPoint2[0] = m_LineCenter[0];
	yPoint1[2] = m_LineCenter[2];
	yPoint2[2] = m_LineCenter[2];

	//z 不变
	zPoint1[2] = m_Spacing[2] * m_Extend[4] + m_Origin[2] + 1;
	zPoint2[2] = m_Spacing[2] * m_Extend[5] + m_Origin[2] + 1;

	zPoint1[0] = m_LineCenter[0];
	zPoint2[0] = m_LineCenter[0];
	zPoint1[1] = m_LineCenter[1];
	zPoint2[1] = m_LineCenter[1];

	for (int i = 0; i < 6; i++)
	{
		//插入每根线的端点
		m_SliceLine[i]->GetPointIds()->InsertNextId(0);
		m_SliceLine[i]->GetPointIds()->InsertNextId(1);
		m_SliceLine[i]->GetPoints()->Initialize();
		switch (i)
		{
		case 0:
			m_SliceLine[i]->GetPoints()->InsertNextPoint(xPoint1);
			m_SliceLine[i]->GetPoints()->InsertNextPoint(xPoint2);
			break;
		case 1:
			m_SliceLine[i]->GetPoints()->InsertNextPoint(yPoint1);
			m_SliceLine[i]->GetPoints()->InsertNextPoint(yPoint2);
			break;
		case 2:
			m_SliceLine[i]->GetPoints()->InsertNextPoint(zPoint1);
			m_SliceLine[i]->GetPoints()->InsertNextPoint(zPoint2);
			break;
		case 3:
			m_SliceLine[i]->GetPoints()->InsertNextPoint(yPoint1);
			m_SliceLine[i]->GetPoints()->InsertNextPoint(yPoint2);
			break;
		case 4:
			m_SliceLine[i]->GetPoints()->InsertNextPoint(zPoint1);
			m_SliceLine[i]->GetPoints()->InsertNextPoint(zPoint2);
			break;
		case 5:
			m_SliceLine[i]->GetPoints()->InsertNextPoint(xPoint1);
			m_SliceLine[i]->GetPoints()->InsertNextPoint(xPoint2);
			break;
		default:
			break;
		}
		//转为单元数据
		auto linecell = vtkCellArray::New();
		linecell->InsertNextCell(m_SliceLine[i]);

		m_SliceLineData[i]->SetLines(linecell);
		m_SliceLineData[i]->SetPoints(m_SliceLine[i]->GetPoints());

		auto linemapper = vtkPolyDataMapper::New();
		linemapper->SetInputData(m_SliceLineData[i]);
		m_SliceLineActor[i]->SetMapper(linemapper);

		linecell->Delete(); linecell = nullptr;
		linemapper->Delete(); linemapper = nullptr;
	}
}
//三维视图初始化
void CTViewer::VolumeRender()
{
	if (m_ImageData == nullptr)
	{
		return;
	}
	//去除原有模型
	if (m_Renderer[3]->GetVolumes()->IsItemPresent(m_Volume))
	{
		m_Renderer[3]->RemoveVolume(m_Volume);
	}

	// delete firstly, then allocate new objects; otherwise, the volume could not display
	m_VolumeMapper->Delete();
	m_VolumeProperty->Delete();
	m_CompositeOpacity->Delete();
	m_ColorTrans->Delete();
	m_Volume->Delete();

	m_VolumeMapper = vtkSmartVolumeMapper::New();
	m_VolumeProperty = vtkVolumeProperty::New();
	m_CompositeOpacity = vtkPiecewiseFunction::New();//分段线性函数
	m_ColorTrans = vtkColorTransferFunction::New();//将一个标量值映射为颜色值
	m_Volume = vtkVolume::New();


	double min = m_VolumeMin;
	double max = m_VolumeMax;
	double level = (min + max) * 0.5;
	m_VolumeMapper->SetInputData(m_ImageData);
	m_VolumeMapper->SetCropping(1);
	m_VolumeMapper->SetCroppingRegionPlanes(m_ImageData->GetBounds());
	m_VolumeMapper->SetCroppingRegionFlags(0x0002000);

	m_VolumeProperty->SetInterpolationTypeToLinear();//设置线性插值
	m_VolumeProperty->ShadeOn();//开启阴影功能
	m_VolumeProperty->SetAmbient(0.4);//设置环境温度系数
	m_VolumeProperty->SetDiffuse(0.6);//设置漫反射系数
	m_VolumeProperty->SetSpecular(0.2);//设置镜面反射系数

	m_CompositeOpacity->RemoveAllPoints();
	m_CompositeOpacity->AddPoint(min, 1);
	m_CompositeOpacity->AddPoint(min + 1, 0.5 * m_Opacity);
	m_CompositeOpacity->AddPoint(level, 0.7 * m_Opacity);
	m_CompositeOpacity->AddPoint(max - 1, 0.8 * m_Opacity);
	m_CompositeOpacity->AddPoint(max, 1 * m_Opacity);

	m_CompositeOpacity->ClampingOff();
	m_VolumeProperty->SetScalarOpacity(m_CompositeOpacity);//设置灰度不透明度函数

//	m_ColorTrans->AddRGBPoint(level, 1.00, 0.52, 0.30);
	m_ColorTrans->AddRGBPoint(min, 0.8, 0.8, 0.8);
	m_ColorTrans->AddRGBPoint(level, 0.8, 0.8, 0.8);
	m_ColorTrans->AddRGBPoint(max, 1.00, 1.00, 1.00);
	m_VolumeProperty->SetColor(m_ColorTrans);//设置颜色传输函数

	m_Volume->SetMapper(m_VolumeMapper);
	m_Volume->SetProperty(m_VolumeProperty);

	m_Renderer[3]->AddVolume(m_Volume);
	for (int i = 0; i < 3; i++)
	{
		m_Renderer[3]->AddViewProp(m_ImageSlice3D[i]);
	}
	m_Renderer[3]->Render();
	m_Renderer[3]->ResetCamera();
	m_RenderWindow[3]->Render();

	m_Renderer[3]->AddVolume(m_Volume);
	m_RenderWindow[3]->Render();
	for (int i = 0; i < 3; ++i)
	{
		m_Renderer[3]->RemoveViewProp(m_ImageSlice3D[i]);
		m_RenderWindow[3]->Render();
	}
}


//layout
void CTViewer::ShowUpLeftView()
{
	//ui.escSagittalButton->hide();
	//ui.SagittalBox->hide();
	ui.SagittalSlider->hide();
	ui.SagittalWidget->hide();

	//ui.escCoronalButton->hide();
	//ui.CoronalBox->hide();
	ui.CoronalSlider->hide();
	ui.CoronalWidget->hide();

	//ui.escModelbutton->hide();
	//ui.ModelBox->hide();
	ui.ModelSlider->hide();
	ui.ModelWidget->hide();

	ui.windowScrollBar->hide();
	//ui.windowBox->hide();
	ui.levelScrollBar->hide();
	//ui.levelBox->hide();

	//ui.escAxialButton->show();
	//ui.AxialBox->show();
	ui.AxialSlider->show();
	ui.AxialWidget->show();
}

void CTViewer::ShowUpRightView()
{
	//ui.escAxialButton->hide();
	//ui.AxialBox->hide();
	ui.AxialSlider->hide();
	ui.AxialWidget->hide();

	//ui.escCoronalButton->hide();
	//ui.CoronalBox->hide();
	ui.CoronalSlider->hide();
	ui.CoronalWidget->hide();

	//ui.escModelbutton->hide();
	//ui.ModelBox->hide();
	ui.ModelSlider->hide();
	ui.ModelWidget->hide();

	ui.windowScrollBar->hide();
	//ui.windowBox->hide();
	ui.levelScrollBar->hide();
	//ui.levelBox->hide();

	//ui.escSagittalButton->show();
	//ui.SagittalBox->show();
	ui.SagittalSlider->show();
	ui.SagittalWidget->show();
}

void CTViewer::ShowLowerLeftView()
{
	//ui.escAxialButton->hide();
	//ui.AxialBox->hide();
	ui.AxialSlider->hide();
	ui.AxialWidget->hide();

	//ui.escSagittalButton->hide();
	//ui.SagittalBox->hide();
	ui.SagittalSlider->hide();
	ui.SagittalWidget->hide();

	//ui.escModelbutton->hide();
	//ui.ModelBox->hide();
	ui.ModelSlider->hide();
	ui.ModelWidget->hide();

	ui.windowScrollBar->hide();
	//ui.windowBox->hide();
	ui.levelScrollBar->hide();
	//ui.levelBox->hide();

	//ui.escCoronalButton->show();
	//ui.CoronalBox->show();
	ui.CoronalSlider->show();
	ui.CoronalWidget->show();
}

void CTViewer::ShowLowerRightView()
{
	//ui.escAxialButton->hide();
	//ui.AxialBox->hide();
	ui.AxialSlider->hide();
	ui.AxialWidget->hide();

	//ui.escSagittalButton->hide();
	//ui.SagittalBox->hide();
	ui.SagittalSlider->hide();
	ui.SagittalWidget->hide();

	//ui.escCoronalButton->hide();
	//ui.CoronalBox->hide();
	ui.CoronalSlider->hide();
	ui.CoronalWidget->hide();

	ui.windowScrollBar->hide();
	//ui.windowBox->hide();
	ui.levelScrollBar->hide();
	//ui.levelBox->hide();

	//ui.escModelbutton->show();
	//ui.ModelBox->show();
	ui.ModelSlider->show();
	ui.ModelWidget->show();
}

void CTViewer::ShowNormal()
{
	//ui.escAxialButton->show();
	//ui.AxialBox->show();
	ui.AxialSlider->show();
	ui.AxialWidget->show();

	//ui.escSagittalButton->show();
	//ui.SagittalBox->show();
	ui.SagittalSlider->show();
	ui.SagittalWidget->show();

	//ui.escCoronalButton->show();
	//ui.CoronalBox->show();
	ui.CoronalSlider->show();
	ui.CoronalWidget->show();

	//ui.escModelbutton->show();
	//ui.ModelBox->show();
	ui.ModelSlider->show();
	ui.ModelWidget->show();

	ui.windowScrollBar->show();
	//ui.windowBox->show();
	ui.levelScrollBar->show();
	//ui.levelBox->show();

}


//Public slots:
void CTViewer::OnSetImageData(vtkImageData* data)
{
	m_ImageData = data;
	m_ImageData->GetExtent(m_Extend);
	m_ImageData->GetSpacing(m_Spacing);
	m_ImageData->GetOrigin(m_Origin);

	this->SetViewerParamters();
	this->VolumeRender();
}

void CTViewer::OnSetMeshFile(QString filepath)
{
	if (filepath.isEmpty())
	{
		return;
	}

	auto stlreader = vtkSTLReader::New();
	stlreader->SetFileName(qPrintable(filepath));
	auto triangleFilter = vtkTriangleFilter::New();
	triangleFilter->SetInputData(stlreader->GetOutput());
	auto mapper = vtkPolyDataMapper::New();
	mapper->SetInputConnection(triangleFilter->GetOutputPort());
	auto meshactor = vtkActor::New();
	meshactor->SetMapper(mapper);
	meshactor->GetProperty()->SetColor(1, 1, 1);

	m_Renderer[3]->AddActor(meshactor);
	m_Renderer[3]->ResetCamera();

	double focalpoint[3];
	double position[3];
	m_Renderer[3]->GetActiveCamera()->GetFocalPoint(focalpoint);
	m_Renderer[3]->GetActiveCamera()->GetPosition(position);
	double dis = sqrt(vtkMath::Distance2BetweenPoints(focalpoint, position));
	m_Renderer[3]->GetActiveCamera()->SetViewUp(0, 0, 1);
	m_Renderer[3]->GetActiveCamera()->SetPosition(focalpoint[0], focalpoint[1] - dis, focalpoint[2]);
	m_Renderer[3]->GetRenderWindow()->Render();

	stlreader->Delete(); stlreader = nullptr;
	triangleFilter->Delete(); triangleFilter = nullptr;
	mapper->Delete(); mapper = nullptr;
	meshactor->Delete(); meshactor = nullptr;
}



void CTViewer::OnSetCoordinateLinesVisible(bool flag)
{
	if (flag)
	{
		for (int i = 0; i < 3; ++i)
		{
			m_SliceLineActor[i]->VisibilityOn();
		}
	}
	else
	{
		for (int i = 0; i < 3; ++i)
		{
			m_SliceLineActor[i]->VisibilityOff();
		}
	}
	m_RenderWindow[0]->Render();
	m_RenderWindow[1]->Render();
	m_RenderWindow[2]->Render();
	m_RenderWindow[3]->Render();
}

void CTViewer::OnSet3DCoordinateLinesVisible(bool flag)
{
	if (flag)
	{
		m_Renderer[3]->AddActor(m_SliceLineActor[0]);
		m_Renderer[3]->AddActor(m_SliceLineActor[1]);
		m_Renderer[3]->AddActor(m_SliceLineActor[2]);
	}
	else
	{
		m_Renderer[3]->RemoveActor(m_SliceLineActor[0]);
		m_Renderer[3]->RemoveActor(m_SliceLineActor[1]);
		m_Renderer[3]->RemoveActor(m_SliceLineActor[2]);
	}
	m_RenderWindow[3]->Render();
}



void CTViewer::OnSetSlicePosition(double x, double y, double z)
{
	double index[3];
	index[0] = (z - m_Origin[2]) / m_Spacing[2];//沿z向层数
	index[1] = (x - m_Origin[0]) / m_Spacing[0];//x
	index[2] = (y - m_Origin[1]) / m_Spacing[1];//y

	//引发视图更新
	ui.AxialSlider->setValue(index[0]);
	ui.SagittalSlider->setValue(index[1]);
	ui.CoronalSlider->setValue(index[2]);
}

void CTViewer::OnResetCamera()
{
	//二维视图
	for (int i = 0; i < 3; ++i)
	{
		switch (i)
		{
		case 0:
			m_Renderer[i]->GetActiveCamera()->SetViewAngle(30);
			m_Renderer[i]->GetActiveCamera()->SetViewUp(0, 1, 0);
			m_Renderer[i]->GetActiveCamera()->SetFocalPoint(m_LineCenter[0], m_LineCenter[1], m_LineCenter[2]);
			m_Renderer[i]->GetActiveCamera()->SetPosition(m_LineCenter[0], m_LineCenter[1], m_LineCenter[2] + 300);
			break;
		case 1:
			m_Renderer[i]->GetActiveCamera()->SetViewAngle(30);
			m_Renderer[i]->GetActiveCamera()->SetViewUp(0, 0, 1);
			m_Renderer[i]->GetActiveCamera()->SetFocalPoint(m_LineCenter[0], m_LineCenter[1], m_LineCenter[2]);
			m_Renderer[i]->GetActiveCamera()->SetPosition(m_LineCenter[0] + 300, m_LineCenter[1], m_LineCenter[2]);
			break;
		case 2:
			m_Renderer[i]->GetActiveCamera()->SetViewAngle(30);
			m_Renderer[i]->GetActiveCamera()->SetViewUp(0, 0, 1);
			m_Renderer[i]->GetActiveCamera()->SetFocalPoint(m_LineCenter[0], m_LineCenter[1], m_LineCenter[2]);
			m_Renderer[i]->GetActiveCamera()->SetPosition(m_LineCenter[0], m_LineCenter[1] - 300, m_LineCenter[2]);
			break;
		default:
			break;
		}
	}
	m_RenderWindow[0]->Render();
	m_RenderWindow[1]->Render();
	m_RenderWindow[2]->Render();

	m_Renderer[3]->ResetCamera();
	double* focusposition = new double[3];
	m_Renderer[3]->GetActiveCamera()->GetFocalPoint(focusposition);

	m_Renderer[3]->GetActiveCamera()->SetViewUp(0, -1, 0);
	m_Renderer[3]->GetActiveCamera()->SetPosition(
		focusposition[0], focusposition[1], focusposition[2] - 400);
	m_Renderer[3]->GetRenderWindow()->Render();

	delete focusposition; focusposition = nullptr;
}

void CTViewer::OnSetZoomIn()
{
	for (int i = 0; i < 3; ++i)
	{
		double angle;
		angle = m_Renderer[i]->GetActiveCamera()->GetViewAngle();
		m_Renderer[i]->GetActiveCamera()->SetViewAngle(angle + 1);
		m_RenderWindow[i]->Render();
	}
}

void CTViewer::OnSetZoomOut()
{
	for (int i = 0; i < 3; ++i)
	{
		double angle;
		angle = m_Renderer[i]->GetActiveCamera()->GetViewAngle();
		m_Renderer[i]->GetActiveCamera()->SetViewAngle(angle - 1);
		m_RenderWindow[i]->Render();
	}
}

void CTViewer::OnSetFicalPointAndZoom(double x, double y, double z, double zoom)
{
	if (zoom > 2 || zoom < 0)
	{
		return;
	}
	m_ZoomFactor = zoom;

	for (int i = 0; i < 3; ++i)
	{
		m_Renderer[i]->GetActiveCamera()->SetFocalPoint(x, y, z);
	}
	m_Renderer[0]->GetActiveCamera()->SetPosition(x, y, z + 100 * (2 - zoom));
	m_Renderer[1]->GetActiveCamera()->SetPosition(x + 100 * (2 - zoom), y, z);
	m_Renderer[2]->GetActiveCamera()->SetPosition(x, y - 100 * (2 - zoom), z);

	this->OnSetSlicePosition(x, y, z);
}

void CTViewer::OnSetRender3FocusPointAndOrientation(double x, double y, double z, double rx, double ry, double rz)
{
	double orien[3] = { rx,ry,rz };
	double focalpoint[3];
	double localpoint[3];
	m_Renderer[3]->GetActiveCamera()->GetFocalPoint(focalpoint);
	m_Renderer[3]->GetActiveCamera()->GetPosition(localpoint);
	m_Renderer[3]->GetActiveCamera()->SetViewUp(0, 0, 1);

	double dis = qSqrt(vtkMath::Distance2BetweenPoints(focalpoint, localpoint));

	orien[2] = 0;
	vtkMath::Normalize(orien);

	for (int i = 0; i < 3; ++i)
	{
		orien[i] = dis * orien[i];
	}

	m_Renderer[3]->GetActiveCamera()->GetFocalPoint(x, y, z);

	m_Renderer[3]->GetActiveCamera()->SetPosition(x + orien[0], y + orien[1], z + orien[2]);
}

vtkSmartPointer<vtkImageData> CTViewer::ThresholdSegment(vtkImageData* data, double lower, double upper)
{
	vtkSmartPointer<vtkImageThreshold> thresholdFilter = vtkSmartPointer<vtkImageThreshold>::New();
	thresholdFilter->SetInputData(data);
	thresholdFilter->ThresholdBetween(lower, upper);
	thresholdFilter->ReplaceInOn();//阈值内的像素值替换
	thresholdFilter->ReplaceOutOn();//阈值外的像素值替换
	thresholdFilter->SetInValue(255);//阈值内像素值全部替换成1
	thresholdFilter->SetOutValue(0);//阈值外像素值全部替换成0
	thresholdFilter->Update();
	return vtkSmartPointer<vtkImageData>(thresholdFilter->GetOutput());
}
/*
//Public slots:
void CTViewer::OnSetImageData(vtkImageData* data)
{
	m_ImageData = data;
	m_ImageData->GetExtent(m_Extend);
	m_ImageData->GetSpacing(m_Spacing);
	m_ImageData->GetOrigin(m_Origin);
	m_ImageData->GetBounds(m_Bound);

	this->SetViewerParamters();
	this->VolumeRender();
}
*/
//private slots:
void CTViewer::OnSetWindowLevel()
{
	m_Window = ui.windowScrollBar->value();
	m_Level = ui.levelScrollBar->value();
	m_ImageSliceProperty->SetColorWindow(m_Window);
	m_ImageSliceProperty->SetColorLevel(m_Level);

	if (m_ImageData == nullptr)
	{
		return;
	}
	for (int i = 0; i < 4; ++i)
	{
		m_RenderWindow[i]->Render();
	}
}

void CTViewer::OnUpdataViews()
{
	this->SetCoodinateLines();//在该函数中更新了plane的位置
	for (int i = 0; i < 4; ++i)
	{
		m_RenderWindow[i]->Render();
	}
}

void CTViewer::OnUpdataLowerRightView()
{
	if (m_Renderer[3]->GetActors()->GetNumberOfItems() < 1)
	{
		return;
	}
	vtkActorCollection* actorCollection = m_Renderer[3]->GetActors();

	int num = actorCollection->GetNumberOfItems();

	//这个函数比较重要，否则getNextActor将没法得到正确的actor
	actorCollection->InitTraversal();

	double opacity = (double)ui.ModelSlider->value() / 100;
	actorCollection->GetNextActor()->GetProperty()->SetOpacity(opacity);
	m_RenderWindow[3]->Render();
}

//鼠标响应
void CTViewer::OnMiddleButton(vtkObject* caller, unsigned long vtk_event)
{
	// get interactor
	vtkRenderWindowInteractor* interactor = vtkRenderWindowInteractor::SafeDownCast(caller);

	if (vtk_event == vtkCommand::MouseWheelForwardEvent)
	{
		if (interactor == m_Interactor[0])
		{
			ui.AxialSlider->setValue(ui.AxialSlider->value() + 1);
		}
		else if (interactor == m_Interactor[1])
		{
			ui.SagittalSlider->setValue(ui.SagittalSlider->value() + 1);
		}
		else if (interactor == m_Interactor[2])
		{
			ui.CoronalSlider->setValue(ui.CoronalSlider->value() + 1);
		}
	}
	else if (vtk_event == vtkCommand::MouseWheelBackwardEvent)
	{
		if (interactor == m_Interactor[0])
		{
			ui.AxialSlider->setValue(ui.AxialSlider->value() - 1);
		}
		else if (interactor == m_Interactor[1])
		{
			ui.SagittalSlider->setValue(ui.SagittalSlider->value() - 1);
		}
		else if (interactor == m_Interactor[2])
		{
			ui.CoronalSlider->setValue(ui.CoronalSlider->value() - 1);
		}
	}
}

void CTViewer::OnLeftButtonPress(vtkObject* caller, unsigned long vtk_event)
{
	// get interactor
	vtkRenderWindowInteractor* interactor = vtkRenderWindowInteractor::SafeDownCast(caller);

	//转换为世界坐标
	auto picker = vtkCellPicker::New();
	interactor->SetPicker(picker);
	picker->Pick(interactor->GetEventPosition()[0],
		interactor->GetEventPosition()[1],
		0,
		interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer());

	double picked[3];
	picker->GetPickPosition(picked);
	this->OnSetSlicePosition(picked[0], picked[1], picked[2]);
	picker->Delete(); picker = nullptr;
}

void CTViewer::OnRightButtonPress(vtkObject* caller, unsigned long vtk_event)
{
	// get interactor
	vtkRenderWindowInteractor* interactor = vtkRenderWindowInteractor::SafeDownCast(caller);

	//转换为世界坐标
	auto picker = vtkCellPicker::New();
	interactor->SetPicker(picker);
	picker->Pick(
		interactor->GetEventPosition()[0],
		interactor->GetEventPosition()[1],
		0,
		interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer());

	double picked[3];
	picker->GetPickPosition(picked);
	this->OnSetSlicePosition(picked[0], picked[1], picked[2]);

	emit sigRightButtonPress(picked[0], picked[1], picked[2]);
	picker->Delete(); picker = nullptr;
}

void CTViewer::OnUpLeftLayout(bool flag)
{
	if (flag)
	{
		this->ShowUpLeftView();
	}
	else
		this->ShowNormal();
}

void CTViewer::OnUpRightLayouy(bool flag)
{
	if (flag)
	{
		this->ShowUpRightView();
	}
	else
		this->ShowNormal();
}

void CTViewer::OnLowerLeftLayout(bool flag)
{
	if (flag)
	{
		this->ShowLowerLeftView();
	}
	else
		this->ShowNormal();
}

void CTViewer::OnLowerRightLayout(bool flag)
{
	if (flag)
	{
		this->ShowLowerRightView();
	}
	else
		this->ShowNormal();
}


//左侧选项区

void CTViewer::OnOpenDicomFile()
{
	//QString filepath = QFileDialog::getOpenFileName(0, "Open a STL file", ".stl");	
	QString filepath = QFileDialog::getExistingDirectory(NULL, "File", ".");
	/*
	if (filepath.isEmpty())
	{
		QMessageBox::warning(0, "Warning", "No File Be Opened");
		return;
	}
	vtkSmartPointer<vtkDICOMImageReader> reader = vtkSmartPointer<vtkDICOMImageReader>::New();
	//reader->SetDirectoryName("SE1");
	//reader->SetDirectoryName("DImage");
	reader->SetDirectoryName(qPrintable(filepath));
	reader->Update();
	//新建一个CDdisplay对象
	//m_CTViewer = new CTViewer(this);
	*/

	typedef itk::GDCMSeriesFileNames NameGeneratorType;
	NameGeneratorType::Pointer namegenerator = NameGeneratorType::New();
	namegenerator->SetUseSeriesDetails(true);
	namegenerator->AddSeriesRestriction("0008|0021");
	namegenerator->SetDirectory(qPrintable(filepath));

	typedef std::vector<std::string> SeriesIdContainer;
	const SeriesIdContainer& seriesUID = namegenerator->GetSeriesUIDs();

	std::string seriesIdentifier;
	seriesIdentifier = seriesUID.begin()->c_str();

	typedef std::vector<std::string> FileNamesContainer;
	FileNamesContainer fileNames;
	fileNames = namegenerator->GetFileNames(seriesIdentifier);

	auto reader = itk::ImageSeriesReader<ITKImage3DType>::New();
	reader->SetImageIO(GDCMIO);
	reader->SetFileNames(fileNames);
	reader->Update();

	//Information

	auto itk2vtk = itk::ImageToVTKImageFilter<ITKImage3DType>::New();
	itk2vtk->SetInput(reader->GetOutput());
	itk2vtk->Update();

	m_vtkImageData = vtkImageData::New();
	m_vtkImageData->DeepCopy(itk2vtk->GetOutput());
	/*
	ui.threshMaxSlider->setMaximum(m_vtkImageData->GetScalarTypeMax());
	ui.threshMaxSlider->setMinimum(m_vtkImageData->GetScalarTypeMin());
	ui.threshMinSlider->setMaximum(m_vtkImageData->GetScalarTypeMax());
	ui.threshMinSlider->setMinimum(m_vtkImageData->GetScalarTypeMin());
	*/
	this->OnSetImageData(m_vtkImageData);
	InputImageDisplay(m_vtkImageData, ui.threshMinSlider->value(),
		ui.threshMaxSlider->value());

}
void CTViewer::onThresholdApply()
{
	SegItem* item = new SegItem(this, this->GetImageData(),
		ui.threshMinSlider->value(), ui.threshMaxSlider->value());
	m_SegItemList.append(item);

	for (int i = 0; i < 3; ++i)
	{
		this->GetRenderer(i)->AddViewProp(item->GetInImageSlice(i));
		this->GetRenderWindow(i)->Render();
	}

	int row = ui.tableWidget->rowCount();
	ui.tableWidget->insertRow(row);

	QColor c_color = item->GetSegItemColor();
	onAddSegItem(row, c_color);
	//OnReconstruction();
}
void CTViewer::onThresholdDelete()
{
	int row = ui.tableWidget->currentRow();
	if (row != -1)

		for (int i = 0; i < 3; ++i)
		{
			this->GetRenderer(i)->RemoveViewProp(m_SegItemList.at(row)->GetInImageSlice(i));
			this->GetRenderWindow(i)->Render();
		}
	ui.tableWidget->removeRow(row);

	//delete m_SegItemList.at(row);
	m_SegItemList.removeAt(row);


	//int volumerow = ui.VolumeTableWidget->currentRow();
	//if (volumerow != -1)
	//{
	//		m_CTViewer->GetRenderer(3)->RemoveVolume(m_VolumeItemList.at(volumerow)->GetVolume());
	//		m_CTViewer->GetRenderWindow(3)->Render();
	//	
	//	ui.VolumeTableWidget->removeRow(volumerow);

	//	//delete m_SegItemList.at(row);
	//	m_VolumeItemList.removeAt(volumerow);
	//}

}
void CTViewer::onAddSegItem(int row, QColor color)
{
	QString name = "No." + QString::number(row + 1);
	ui.tableWidget->setItem(row, 0, new QTableWidgetItem(name));//name
	ui.tableWidget->item(row, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);//居中

	QCheckBox* VisableBox = new	QCheckBox();
	VisableBox->setChecked(true);
	ui.tableWidget->setCellWidget(row, 1, VisableBox);//visable
	//connect(VisableBox, SIGNAL(clicked()), this, SLOT(onSegItemColorChange()));
	connect(VisableBox, SIGNAL(stateChanged(int)), this, SLOT(onSegItemVisable(int)));

	QPushButton* ColorButton = new QPushButton();
	ui.tableWidget->setCellWidget(row, 2, ColorButton);//color
	QString stylesheet = QString("background-color:rgb(%1,%2,%3)").arg(color.red()).arg(color.green()).arg(color.blue());
	ColorButton->setStyleSheet(stylesheet);
	connect(ColorButton, SIGNAL(clicked()), this, SLOT(onSegItemColorChange()));

	QLineEdit* EditLower = new QLineEdit("");
	EditLower->setValidator(new QIntValidator(0, 255, this));
	ui.tableWidget->setCellWidget(row, 3, EditLower);//lower
	EditLower->setStyleSheet("background:transparent;border-width:0;border-style:outset");
	m_SegItemList.at(row)->OnSetLowerEdit(EditLower);

	QLineEdit* EditHigher = new QLineEdit("");
	EditHigher->setValidator(new QIntValidator(0, 255, this));
	ui.tableWidget->setCellWidget(row, 4, EditHigher);//higher
	EditHigher->setStyleSheet("background:transparent;border-width:0;border-style:outset");
	m_SegItemList.at(row)->OnSetHigherEdit(EditHigher);

	QString lowertext = QString("%1").arg(ui.threshMinSlider->value());
	QString highertext = QString("%1").arg(ui.threshMaxSlider->value());
	EditLower->setText(lowertext);
	EditHigher->setText(highertext);
	connect(EditLower, SIGNAL(editingFinished()), this, SLOT(onSegItemLowHigh()));
	connect(EditHigher, SIGNAL(editingFinished()), this, SLOT(onSegItemLowHigh()));


	//m_SegItemList.at(row)->OnGetLowerEdit()->setText(lowertext);
	//m_SegItemList.at(row)->OnGetHigherEdit()->setText(highertext);

}
void CTViewer::onSegItemColorChange()
{
	QColor color = QColorDialog::getColor(Qt::red, this,
		"Choose color for the segmentation label",
		QColorDialog::ShowAlphaChannel);

	QString stylesheet = QString("background-color:rgb(%1,%2,%3)").arg(color.red()).arg(color.green()).arg(color.blue());
	int row = ui.tableWidget->currentRow();
	ui.tableWidget->cellWidget(row, 2)->setStyleSheet(stylesheet);
	m_SegItemList.at(row)->OnSetColor(color);
}
void CTViewer::onSegItemVisable(int state)
{
	int row = ui.tableWidget->currentRow();
	if (state == Qt::Checked) // "选中"
	{
		for (int i = 0; i < 3; ++i)
		{
			this->GetRenderer(i)->AddViewProp(m_SegItemList.at(row)->GetInImageSlice(i));
			this->GetRenderWindow(i)->Render();

		}
	}
	else // 未选中 - Qt::Unchecked
	{
		for (int i = 0; i < 3; ++i)
		{
			this->GetRenderer(i)->RemoveViewProp(m_SegItemList.at(row)->GetInImageSlice(i));
			this->GetRenderWindow(i)->Render();
		}
	}
}
void CTViewer::onPreview(int state)
{
	if (state == Qt::Checked) // "选中"
	{
		for (int i = 0; i < 3; ++i)
		{
			this->GetRenderer(i)->AddViewProp(InImageSlice[i]);
			this->GetRenderWindow(i)->Render();

		}
	}
	else // 未选中 - Qt::Unchecked
	{
		for (int i = 0; i < 3; ++i)
		{
			this->GetRenderer(i)->RemoveViewProp(InImageSlice[i]);
			this->GetRenderWindow(i)->Render();
		}
	}
}
void CTViewer::onDisable(int state)
{
	if (state == Qt::Checked) // "选中"
	{
		m_Renderer[3]->RemoveVolume(m_Volume);
		m_RenderWindow[3]->Render();
	}
	else // 未选中 - Qt::Unchecked
	{
		m_Renderer[3]->AddVolume(m_Volume);
		m_RenderWindow[3]->Render();
	}
}

void CTViewer::onSegItemLowHigh()
{
	int row = ui.tableWidget->currentRow();
	QString lowertext;
	QString hightext;

	int lower = m_SegItemList.at(row)->OnGetLowerEdit()->text().toInt();
	int higher = m_SegItemList.at(row)->OnGetHigherEdit()->text().toInt();

	//ui.spinBox->setValue(lower);
	m_SegItemList.at(row)->OnSetLimit(lower, higher);
	for (int i = 0; i < 3; ++i)
	{
		//this->GetRenderer(i)->AddViewProp(m_SegItemList.at(row)->GetInImageSlice(i));
		this->GetRenderWindow(i)->Render();
	}

}
void CTViewer::InputImageDisplay(vtkImageData* image, int lower, int higher)
{
	m_thresholdFilter = vtkSmartPointer<vtkImageThreshold>::New();
	m_thresholdFilter->SetInputData(image);
	m_thresholdFilter->ThresholdBetween(lower, higher);
	m_thresholdFilter->ReplaceInOn();//阈值内的像素值替换
	m_thresholdFilter->ReplaceOutOn();//阈值外的像素值替换
	m_thresholdFilter->SetInValue(255);//阈值内像素值全部替换成1
	m_thresholdFilter->SetOutValue(0);//阈值外像素值全部替换成0
	m_thresholdFilter->Update();
	//m_SegItemData = vtkSmartPointer<vtkImageData>(m_thresholdFilter->GetOutput());
	vtkSmartPointer<vtkLookupTable> m_ColorTable = vtkSmartPointer<vtkLookupTable>::New();
	m_ColorTable->SetNumberOfColors(2);
	m_ColorTable->SetTableRange(0, 255);
	m_ColorTable->SetTableValue(0, 0.0, 0.0, 0.0, 0.0);
	m_ColorTable->SetTableValue(1, 0, 1, 0, 1.0);//1, 1, 0,
	m_ColorTable->Build();

	vtkImageProperty* m_ImageSlicePropertyLayer = vtkImageProperty::New();
	m_ImageSlicePropertyLayer->SetInterpolationTypeToLinear();
	m_ImageSlicePropertyLayer->SetLookupTable(m_ColorTable);
	m_ImageSlicePropertyLayer->SetOpacity(1);
	for (int i = 0; i < 3; ++i)
	{
		vtkSmartPointer<vtkImageResliceMapper> m_InResliceMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
		InImageSlice[i] = vtkImageSlice::New();

		m_InResliceMapper->SetInputData(m_thresholdFilter->GetOutput());//m_SegItemData
		m_InResliceMapper->SetSlicePlane(this->GetPlane(i));
		InImageSlice[i]->SetMapper(m_InResliceMapper);
		InImageSlice[i]->SetProperty(m_ImageSlicePropertyLayer);
		//this->GetRenderer(i)->AddViewProp(InImageSlice[i]);
	}
}
void CTViewer::OnLimitChange()
{
	m_thresholdFilter->ThresholdBetween(ui.threshMinSlider->value(), ui.threshMaxSlider->value());
	m_thresholdFilter->Update();

	for (int i = 0; i < 3; ++i)
	{
		InImageSlice[i]->Update();
		this->GetRenderWindow(i)->Render();
	}

}

//for RG
bool CTViewer::CheckSeedPointInsideImage(double worldSeedPoint[3])
{
	typedef itk::Point<double, Image3DType::ImageDimension> PointType;
	PointType point;
	point[0] = worldSeedPoint[0];    // x coordinate
	point[1] = worldSeedPoint[1];    // y coordinate
	point[2] = worldSeedPoint[2];    // z coordinate

	//根据选取的种子点的世界坐标，获取这个种子点在itk图像中的像素索引值.
	bool isInside = __mItkImage->TransformPhysicalPointToIndex(point, __mPixelIndex);
	return isInside;
}

void CTViewer::OnRG()
{
	int imagerow = ui.tableWidget->currentRow();
	vtkImageData* __originalImageData = m_SegItemList.at(imagerow)->GetSegItemData();

	__lowerThreshold = 255;//ui.threshMinSlider->value();
	__upperThreshold = 255;//ui.threshMaxSlider->value();
	__replaceValue = 255;
	__mItkImage = Image3DType::New();
	__connectedThreshold = ConnectedFilterType::New();
	__connector = ConnectorType::New();

	double imageSpacing[3] = { 0.0 };
	double imageOrigin[3] = { 0.0 };
	int imageDimension[3] = { 0 };
	__originalImageData->GetSpacing(imageSpacing);
	__originalImageData->GetOrigin(imageOrigin);
	__originalImageData->GetDimensions(imageDimension);

	const Image3DType::SizeType  size = { {imageDimension[0], imageDimension[1], imageDimension[2]} }; //Size along {X,Y,Z}
	const Image3DType::IndexType start = { {0,0,0} }; // First index on {X,Y,Z}

	Image3DType::RegionType region;
	region.SetSize(size);
	region.SetIndex(start);

	//Types for converting between ITK and VTK
	typedef itk::VTKImageToImageFilter<Image3DType> VTKImageToImage3DTypeFilter;

	//Converting to ITK Image Format
	VTKImageToImage3DTypeFilter::Pointer vtkImageToImage3DTypeFilter = VTKImageToImage3DTypeFilter::New();
	vtkImageToImage3DTypeFilter->SetInput(__originalImageData);
	vtkImageToImage3DTypeFilter->UpdateLargestPossibleRegion();

	//将vtk的图像转化为itk的图像，以便利用itk的分割算法进行分割.
	__mItkImage->SetRegions(region);

	Image3DType::SpacingType spacing;
	spacing[0] = imageSpacing[0]; // spacing along X
	spacing[1] = imageSpacing[1]; // spacing along Y
	spacing[2] = imageSpacing[2]; // spacing along Z
	__mItkImage->SetSpacing(spacing);

	Image3DType::PointType newOrigin;
	newOrigin[0] = imageOrigin[0];
	newOrigin[1] = imageOrigin[1];
	newOrigin[2] = imageOrigin[2];
	__mItkImage->SetOrigin(newOrigin);

	Image3DType::DirectionType direction;
	direction.SetIdentity();
	__mItkImage->SetDirection(direction);

	__mItkImage->Allocate();
	__mItkImage = const_cast<Image3DType*>(vtkImageToImage3DTypeFilter->GetImporter()->GetOutput());

	__mItkImage->UpdateOutputInformation();

	bool isSeedPointInsideImage = CheckSeedPointInsideImage(m_LineCenter);
	if (!isSeedPointInsideImage)
	{
		return;
	}

	__connectedThreshold->SetInput(__mItkImage);
	__connectedThreshold->SetLower(__lowerThreshold);
	__connectedThreshold->SetUpper(__upperThreshold);
	__connectedThreshold->SetReplaceValue(__replaceValue);
	__connectedThreshold->SetSeed(__mPixelIndex);

	//Converting Back from ITK to VTK Image for Visualization.
	__connector->SetInput(__connectedThreshold->GetOutput());
	__connector->Update();
	__outputImageData = __connector->GetOutput();

	SegItem* item = new SegItem(this, __outputImageData, 255, 255);
	m_SegItemList.append(item);

	for (int i = 0; i < 3; ++i)
	{
		this->GetRenderer(i)->AddViewProp(item->GetInImageSlice(i));
		this->GetRenderWindow(i)->Render();
	}

	int row = ui.tableWidget->rowCount();
	ui.tableWidget->insertRow(row);

	QColor c_color = item->GetSegItemColor();
	onAddSegItem(row, c_color);
}


void CTViewer::OnReconstruction()
{
	int imagerow = ui.tableWidget->currentRow();
	//m_SegItemList.at(imagerow)->GetSegItemData();
	vtkImageData* segdata = m_SegItemList.at(imagerow)->GetSegItemData();
	QColor color = m_SegItemList.at(imagerow)->GetSegItemColor();
	VolumeItem* volumeitem = new VolumeItem(this,
		segdata, color);
	m_VolumeItemList.append(volumeitem);

	//this->GetRenderer(3)->AddVolume(volumeitem->GetVolume());
	this->GetRenderer(3)->AddActor(volumeitem->GetVolume());
	this->GetRenderWindow(3)->Render();
	
	int volumerow = ui.VolumeTableWidget->rowCount();
	ui.VolumeTableWidget->insertRow(volumerow);

	QColor c_color = volumeitem->GetVolItemColor();
	onAddVolumeItem(volumerow, c_color);
}
void CTViewer::onAddVolumeItem(int row, QColor color)
{
	QString name = "No." + QString::number(row + 1);
	ui.VolumeTableWidget->setItem(row, 0, new QTableWidgetItem(name));//name
	ui.VolumeTableWidget->item(row, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);//居中

	QCheckBox* VisableBox = new	QCheckBox();
	VisableBox->setChecked(true);
	ui.VolumeTableWidget->setCellWidget(row, 1, VisableBox);//visable
	//connect(VisableBox, SIGNAL(clicked()), this, SLOT(onSegItemColorChange()));
	connect(VisableBox, SIGNAL(stateChanged(int)), this, SLOT(onVolumeItemVisable(int)));

	QPushButton* ColorButton = new QPushButton();
	ui.VolumeTableWidget->setCellWidget(row, 2, ColorButton);//color
	QString stylesheet = QString("background-color:rgb(%1,%2,%3)").arg(color.red()).arg(color.green()).arg(color.blue());
	ColorButton->setStyleSheet(stylesheet);
	connect(ColorButton, SIGNAL(clicked()), this, SLOT(onVolumeColorChange()));
	/*
	QSlider* slider = new QSlider(Qt::Horizontal);
	slider->setMinimum(0);
	slider->setMaximum(100);
	slider->setValue(80);
	m_VolumeItemList.at(row)->OnSetSlider(slider);
	ui.VolumeTableWidget->setCellWidget(row, 3, slider);//color
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSetVolumeOpacity(int)));
	*/
	QPushButton* ExportButton = new QPushButton();
	ui.VolumeTableWidget->setCellWidget(row, 3, ExportButton);
	ExportButton->setText("STL");
	ExportButton->setStyleSheet("background-color:rgb(255,255,255)");
	connect(ExportButton, SIGNAL(clicked()), this, SLOT(onExportSTL()));
}
void CTViewer::onSetVolumeOpacity(int value)
{
	int row = ui.VolumeTableWidget->currentRow();
	//double opacity =( m_VolumeItemList.at(row)->OnGetSlider()->value() )/ 100;
	double opacity = (double)value / (double)100.0;
	m_VolumeItemList.at(row)->OnSetOpacity(opacity);
}

void CTViewer::onVolumeItemVisable(int state)
{
	int row = ui.VolumeTableWidget->currentRow();
	if (state == Qt::Checked) // "选中"
	{
		//this->GetRenderer(3)->AddVolume(m_VolumeItemList.at(row)->GetVolume());
		this->GetRenderer(3)->AddActor(m_VolumeItemList.at(row)->GetVolume());
		this->GetRenderWindow(3)->Render();
	}
	else // 未选中 - Qt::Unchecked
	{
		//this->GetRenderer(3)->RemoveVolume(m_VolumeItemList.at(row)->GetVolume());
		this->GetRenderer(3)->RemoveActor(m_VolumeItemList.at(row)->GetVolume());
		this->GetRenderWindow(3)->Render();
	}
}
void CTViewer::onVolumeColorChange()
{
	QColor color = QColorDialog::getColor(Qt::red, this,
		"Choose color for the volume",
		QColorDialog::ShowAlphaChannel);

	QString stylesheet = QString("background-color:rgb(%1,%2,%3)").arg(color.red()).arg(color.green()).arg(color.blue());
	int row = ui.VolumeTableWidget->currentRow();
	ui.VolumeTableWidget->cellWidget(row, 2)->setStyleSheet(stylesheet);
	m_VolumeItemList.at(row)->OnSetColor(color);
}

void CTViewer::onExportSTL()
{
	std::string filename = "out.stl";

	int row = ui.VolumeTableWidget->currentRow();
	vtkSmoothPolyDataFilter* VolumeFilter = m_VolumeItemList.at(row)->GetFilter();

	vtkSmartPointer<vtkSTLWriter> stlWriter =
		vtkSmartPointer<vtkSTLWriter>::New();
	stlWriter->SetFileName(filename.c_str());
	stlWriter->SetInputConnection(VolumeFilter->GetOutputPort());
	stlWriter->SetFileTypeToBinary();
	stlWriter->Write();
	stlWriter->Update();
}

//——————————————————————————————

SegItem::SegItem(CTViewer* CTViewer, vtkImageData* data, int lower, int higher)
{
	m_lower = lower;
	m_higher = higher;
	m_InputImageData = data;
	m_CTViewer = CTViewer;
	int m_RdColor[3] = { RandomCreatFunc(0, 255), RandomCreatFunc(0, 255), RandomCreatFunc(0, 255) };
	m_Color.setRgb(m_RdColor[0], m_RdColor[1], m_RdColor[2], 255);
	//m_SegItemData = ThresholdSegment(m_InputImageData, m_lower, m_higher);
	InputImageDisplay();
}
SegItem::~SegItem()
{

	/*m_InputImageData->Delete();
	m_SegItemData->Delete();*/
	//QColor m_Color;
	//int m_lower;
	////int m_higher;
	//m_ColorTable->Delete();
	//thresholdFilter->Delete();
	//m_ColorTable = nullptr;
	//thresholdFilter = nullptr;
	//
	//for (int i = 0; i < 3; ++i)
	//{
	//	 InImageSlice[i]->Delete();
	//	 InImageSlice[i] = nullptr;
	//	
	//}
	//
	/*m_EditLower->;
	QLineEdit* m_EditHigher;*/

}
vtkSmartPointer<vtkImageData> SegItem::ThresholdSegment(vtkImageData* data, double lower, double higher)
{
	thresholdFilter = vtkSmartPointer<vtkImageThreshold>::New();
	thresholdFilter->SetInputData(data);
	thresholdFilter->ThresholdBetween(lower, higher);
	thresholdFilter->ReplaceInOn();//阈值内的像素值替换
	thresholdFilter->ReplaceOutOn();//阈值外的像素值替换
	thresholdFilter->SetInValue(255);//阈值内像素值全部替换成1
	thresholdFilter->SetOutValue(0);//阈值外像素值全部替换成0
	thresholdFilter->Update();
	return vtkSmartPointer<vtkImageData>(thresholdFilter->GetOutput());
}

void SegItem::InputImageDisplay()
{
	thresholdFilter = vtkSmartPointer<vtkImageThreshold>::New();
	thresholdFilter->SetInputData(m_InputImageData);
	thresholdFilter->ThresholdBetween(m_lower, m_higher);
	thresholdFilter->ReplaceInOn();//阈值内的像素值替换
	thresholdFilter->ReplaceOutOn();//阈值外的像素值替换
	thresholdFilter->SetInValue(255);//阈值内像素值全部替换成1
	thresholdFilter->SetOutValue(0);//阈值外像素值全部替换成0
	thresholdFilter->Update();
	/*m_SegItemData = vtkImageData::New();
	m_SegItemData = thresholdFilter->GetOutput();*/
	m_SegItemData = vtkSmartPointer<vtkImageData>(thresholdFilter->GetOutput());
	m_ColorTable = vtkSmartPointer<vtkLookupTable>::New();
	m_ColorTable->SetNumberOfColors(2);
	m_ColorTable->SetTableRange(0, 255);
	m_ColorTable->SetTableValue(0, 0.0, 0.0, 0.0, 0.0);
	m_ColorTable->SetTableValue(1, m_Color.redF(), m_Color.greenF(), m_Color.blueF(), 1.0);//1, 1, 0,
	m_ColorTable->Build();

	vtkImageProperty* m_ImageSlicePropertyLayer = vtkImageProperty::New();
	m_ImageSlicePropertyLayer->SetInterpolationTypeToLinear();
	m_ImageSlicePropertyLayer->SetLookupTable(m_ColorTable);
	m_ImageSlicePropertyLayer->SetOpacity(0.7);
	for (int i = 0; i < 3; ++i)
	{
		vtkSmartPointer<vtkImageResliceMapper> m_InResliceMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
		InImageSlice[i] = vtkImageSlice::New();

		m_InResliceMapper->SetInputData(thresholdFilter->GetOutput());//m_SegItemData
		m_InResliceMapper->SetSlicePlane(m_CTViewer->GetPlane(i));
		InImageSlice[i]->SetMapper(m_InResliceMapper);
		InImageSlice[i]->SetProperty(m_ImageSlicePropertyLayer);
	}
}

void SegItem::OnSetColor(QColor color)
{
	m_Color = color;
	m_ColorTable->SetTableValue(1, m_Color.redF(), m_Color.greenF(), m_Color.blueF(), 1.0);//1, 1, 0,
	//m_ColorTable->Build();

	for (int i = 0; i < 3; ++i)
	{
		InImageSlice[i]->Update();
	}
}
void SegItem::OnSetLimit(int lower, int higher)
{
	m_lower = lower;
	m_higher = higher;
	thresholdFilter->ThresholdBetween(lower, higher);
	thresholdFilter->Update();

	for (int i = 0; i < 3; ++i)
	{
		InImageSlice[i]->Update();
	}

}
QColor SegItem::GetSegItemColor()
{
	return m_Color;
}
vtkSmartPointer<vtkImageData> SegItem::GetSegItemData()
{
	return m_SegItemData;
}
vtkImageSlice* SegItem::GetInImageSlice(int i)
{
	return InImageSlice[i];
}

void SegItem::OnSetLowerEdit(QLineEdit* EditLower)
{
	m_EditLower = EditLower;
}
void SegItem::OnSetHigherEdit(QLineEdit* EditHigher)
{
	m_EditHigher = EditHigher;
}
QLineEdit* SegItem::OnGetLowerEdit()
{
	return m_EditLower;
}
QLineEdit* SegItem::OnGetHigherEdit()
{
	return m_EditHigher;
}


//——————————————————————————————————————

VolumeItem::VolumeItem(CTViewer* CTViewer1, vtkImageData* ddata, QColor color)
{
	m_VolumeImageData = ddata;
	m_CTViewer1 = CTViewer1;
	m_Color1 = color;
	//m_Color1.setRgb(0, 255, 0, 255);

	m_VolumeMin1 = 0;
	m_VolumeMax1 = 1500;
	m_Opacity1 = 1;
	m_level1 = (m_VolumeMin1 + m_VolumeMax1) * 0.5;

	ReconstructVolume();
}
VolumeItem::~VolumeItem()
{
}

/*
void CTViewer::OnReconstruction()
{
	int imagerow = ui.tableWidget->currentRow();
	vtkImageData* segdata = m_SegItemList.at(imagerow)->GetSegItemData();

	vtkSmartPointer<vtkImageMarchingCubes> surface =
		vtkSmartPointer<vtkImageMarchingCubes>::New();
	surface->SetInputData(segdata);
	surface->ComputeNormalsOn();
	surface->SetValue(0, 100);

	vtkSmartPointer<vtkPolyDataMapper> surfMapper =
		vtkSmartPointer<vtkPolyDataMapper>::New();
	surfMapper->SetInputConnection(surface->GetOutputPort());
	surfMapper->ScalarVisibilityOff();

	vtkSmartPointer<vtkActor> surfActor =
		vtkSmartPointer<vtkActor>::New();
	surfActor->SetMapper(surfMapper);
	surfActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

	m_Renderer[3]->AddActor(surfActor);
	m_RenderWindow[3]->Render();
}
*/
void VolumeItem::ReconstructVolume()
{
	vtkSmartPointer<vtkImageMarchingCubes> surface =
		vtkSmartPointer<vtkImageMarchingCubes>::New();
	surface->SetInputData(m_VolumeImageData);
	surface->ComputeNormalsOn();
	surface->SetValue(0, 100);

	vtkSmartPointer<vtkSmoothPolyDataFilter> smoothFilter =
		vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
	smoothFilter->SetInputConnection(surface->GetOutputPort());
	smoothFilter->SetRelaxationFactor(0.01);
	smoothFilter->SetNumberOfIterations(10);
	smoothFilter->FeatureEdgeSmoothingOff();
	smoothFilter->BoundarySmoothingOff();
	smoothFilter->Update();

	m_Filter1 = smoothFilter;

	/*
	vtkSmartPointer<vtkWindowedSincPolyDataFilter> smoothFilter =
		vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
	smoothFilter->SetInputConnection(surface->GetOutputPort());
	smoothFilter->SetNumberOfIterations(10);
	smoothFilter->Update();
	*/

	vtkSmartPointer<vtkPolyDataMapper> surfMapper =
		vtkSmartPointer<vtkPolyDataMapper>::New();
	surfMapper->SetInputConnection(smoothFilter->GetOutputPort());
	surfMapper->ScalarVisibilityOff();

	vtkSmartPointer<vtkActor> m_Volume1 =
		vtkSmartPointer<vtkActor>::New();
	m_Volume1->SetMapper(surfMapper);

	m_VolumeProperty1 = vtkProperty::New();
	m_VolumeProperty1->SetColor(m_Color1.redF(), m_Color1.greenF(), m_Color1.blueF());
	/*
	m_CompositeOpacity1 = vtkPiecewiseFunction::New();//分段线性函数
	m_ColorTrans1 = vtkColorTransferFunction::New();//将一个标量值映射为颜色值

	m_VolumeProperty1->SetInterpolationToFlat();//设置线性插值
	//m_VolumeProperty1->ShadeOn();//开启阴影功能
	m_VolumeProperty1->SetAmbient(0.4);//设置环境温度系数
	m_VolumeProperty1->SetDiffuse(0.6);//设置漫反射系数
	m_VolumeProperty1->SetSpecular(0.2);//设置镜面反射系数

	m_CompositeOpacity1->RemoveAllPoints();
	m_CompositeOpacity1->AddPoint(m_VolumeMin1, 1);
	m_CompositeOpacity1->AddPoint(m_VolumeMin1 + 1, 0.5 * m_Opacity1);
	m_CompositeOpacity1->AddPoint(m_level1, 0.7 * m_Opacity1);
	m_CompositeOpacity1->AddPoint(m_VolumeMax1 - 1, 0.8 * m_Opacity1);
	m_CompositeOpacity1->AddPoint(m_VolumeMax1, 1 * m_Opacity1);
	m_CompositeOpacity1->ClampingOff();
	m_VolumeProperty1->SetOpacity(m_CompositeOpacity1);//设置灰度不透明度函数


	m_ColorTrans1->AddRGBPoint(m_VolumeMin1, 0, 0.8, 0);
	m_ColorTrans1->AddRGBPoint(m_level1, 0, 0.8, 0);
	m_ColorTrans1->AddRGBPoint(m_VolumeMax1, 0.00, 1.00, 0.00);
	m_VolumeProperty1->SetColor(m_ColorTrans1);//设置颜色传输函数
	*/

	m_Volume1->SetProperty(m_VolumeProperty1);
	m_CTViewer1->GetRenderer(3)->AddActor(m_Volume1);
}

//vtkVolume* VolumeItem::GetVolume()
vtkActor* VolumeItem::GetVolume()
{
	return m_Volume1;
}
void VolumeItem::OnSetColor(QColor color)
{
	m_Color1 = color;
	//m_ColorTrans1->RemoveAllPoints();
	//m_ColorTrans1->AddRGBPoint(m_VolumeMin1, 0.8 * m_Color1.redF(), 0.8 * m_Color1.greenF(), 0.8 * m_Color1.blueF());
	//m_ColorTrans1->AddRGBPoint(m_level1, 0.8 * m_Color1.redF(), 0.8 * m_Color1.greenF(), 0.8 * m_Color1.blueF());
	//m_ColorTrans1->AddRGBPoint(m_VolumeMax1, m_Color1.redF(), m_Color1.greenF(), m_Color1.blueF());
	m_VolumeProperty1->SetColor(m_Color1.redF(), m_Color1.greenF(), m_Color1.blueF());//设置颜色传输函数

	m_CTViewer1->GetRenderWindow(3)->Render();
}
void VolumeItem::OnSetOpacity(double opacity)
{
	m_Opacity1 = opacity;
	m_VolumeProperty1->SetOpacity(m_Opacity1);

	//m_CompositeOpacity1->RemoveAllPoints();
	//m_CompositeOpacity1->AddPoint(m_VolumeMin1, 0.3 * m_Opacity1);
	//m_CompositeOpacity1->AddPoint(m_VolumeMin1 + 1, 0.5 * m_Opacity1);
	//m_CompositeOpacity1->AddPoint(m_level1, 0.7 * m_Opacity1);
	//m_CompositeOpacity1->AddPoint(m_VolumeMax1 - 1, 0.8 * m_Opacity1);
	//m_CompositeOpacity1->AddPoint(m_VolumeMax1, 1 * m_Opacity1);
	//m_CompositeOpacity1->ClampingOff();
	//m_VolumeProperty1->SetScalarOpacity(m_CompositeOpacity1);//设置灰度不透明度函数
	m_CTViewer1->GetRenderWindow(3)->Render();
}
void VolumeItem::OnSetSlider(QSlider* slider)
{
	m_slider = slider;
}
QSlider* VolumeItem::OnGetSlider()
{
	return m_slider;
}
QColor VolumeItem::GetVolItemColor()
{
	return m_Color1;
}

vtkSmoothPolyDataFilter* VolumeItem::GetFilter()
{
	return m_Filter1;
}
