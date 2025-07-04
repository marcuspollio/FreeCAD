/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QContextMenuEvent>
#include <QImage>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QShortcut>
#include <QString>
#include <QWidgetAction>
#include <boost/core/ignore_unused.hpp>
#include <limits>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Notifications.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "TaskSketcherElements.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "ui_TaskSketcherElements.h"

// clang-format off
using namespace SketcherGui;
using namespace Gui::TaskView;

// Translation block for context menu: do not remove
#if 0
QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Point Coincidence");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Point on Object");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Vertical Constraint");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Horizontal Constraint");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Parallel Constraint");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Perpendicular Constraint");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Tangent Constraint");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Equal Constraint");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Symmetric Constraint");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Block Constraint");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Lock Position");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Horizontal Dimension");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Vertical Dimension");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Length Dimension");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Radius Dimension");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Diameter Dimension");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Radius or Diameter Dimension");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Angle Dimension");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Construction Geometry");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Select Constraints");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Select Origin");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Select Horizontal Axis");

QT_TRANSLATE_NOOP("SketcherGui::ElementView", "Select Vertical Axis");

#endif

/// Inserts a QAction into an existing menu
/// ICONSTR is the string of the icon in the resource file
/// NAMESTR is the text appearing in the contextual menuAction
/// CMDSTR is the string registered in the commandManager
/// FUNC is the name of the member function to be executed on selection of the menu item
/// ACTSONSELECTION is a true/false value to activate the command only if a selection is made
#define CONTEXT_ITEM(ICONSTR, NAMESTR, CMDSTR, FUNC, ACTSONSELECTION)                              \
    QIcon icon_##FUNC(Gui::BitmapFactory().pixmap(ICONSTR));                                       \
    QAction* constr_##FUNC = menu.addAction(icon_##FUNC, tr(NAMESTR), this, SLOT(FUNC()));         \
    constr_##FUNC->setShortcut(QKeySequence(QString::fromUtf8(                                     \
        Gui::Application::Instance->commandManager().getCommandByName(CMDSTR)->getAccel())));      \
    if (ACTSONSELECTION)                                                                           \
        constr_##FUNC->setEnabled(!items.isEmpty());                                               \
    else                                                                                           \
        constr_##FUNC->setEnabled(true);

/// Defines the member function corresponding to the CONTEXT_ITEM macro
#define CONTEXT_MEMBER_DEF(CMDSTR, FUNC)                                                           \
    void ElementView::FUNC()                                                                       \
    {                                                                                              \
        Gui::Application::Instance->commandManager().runCommandByName(CMDSTR);                     \
    }


namespace SketcherGui
{

class ElementItemDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    /// Enum containing all controls rendered in this item. Controls in that enum MUST be in order.
    enum SubControl : int {
        CheckBox,
        LineSelect,
        StartSelect,
        EndSelect,
        MidSelect,
        Label
    };

    explicit ElementItemDelegate(ElementView* parent);
    ~ElementItemDelegate() override = default;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

    ElementItem* getElementItem(const QModelIndex& index) const;

    QRect subControlRect(SubControl element, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void drawSubControl(SubControl element, QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    const int gap = 4; // 4px of spacing between consecutive elements

Q_SIGNALS:
    void itemHovered(QModelIndex);
    void itemChecked(QModelIndex, Qt::CheckState state);
};

// clang-format on
// helper class to store additional information about the listWidget entry.
class ElementItem: public QListWidgetItem
{
public:
    enum class GeometryState
    {
        Normal,
        Construction,
        InternalAlignment,
        External
    };

    enum class Layer
    {
        Default = 0,
        Discontinuous = 1,
        Hidden = 2,
    };

    ElementItem(int elementnr,
                int startingVertex,
                int midVertex,
                int endVertex,
                Base::Type geometryType,
                GeometryState state,
                const QString& lab,
                ViewProviderSketch* sketchView)
        : ElementNbr(elementnr)
        , StartingVertex(startingVertex)
        , MidVertex(midVertex)
        , EndVertex(endVertex)
        , GeometryType(std::move(geometryType))
        , State(state)
        , isLineSelected(false)
        , isStartingPointSelected(false)
        , isEndPointSelected(false)
        , isMidPointSelected(false)
        , clickedOn(SubElementType::none)
        , hovered(SubElementType::none)
        , rightClicked(false)
        , label(lab)
        , sketchView(sketchView)
    {}

    ~ElementItem() override
    {}

    bool canBeHidden() const
    {
        return State != GeometryState::External;
    }

    bool isVisible() const
    {
        if (State != GeometryState::External) {
            const auto geo = sketchView->getSketchObject()->getGeometry(ElementNbr);
            if (geo) {
                auto layer = getSafeGeomLayerId(geo);

                return layer != static_cast<unsigned int>(Layer::Hidden);
            }
        }

        // 1. external geometry currently is always visible.
        // 2. if internal and ElementNbr is out of range, the element
        // needs to be updated and the return value is not important.
        return true;
    }

    QVariant data(int role) const override
    {
        // In order for content-box to include size of the 4 geometry icons we need to provide
        // Qt with information about decoration (icon) size. This is hack to work around Qt
        // limitation of not knowing about padding, border and margin boxes of stylesheets
        // thus being unable to provide proper sizeHint for stylesheets to render correctly
        if (role == Qt::DecorationRole) {
            auto size = listWidget()->iconSize();

            return QIcon(QPixmap(size));
        }

        return QListWidgetItem::data(role);
    }

    bool isGeometrySelected(Sketcher::PointPos pos) const
    {
        switch (pos) {
            case Sketcher::PointPos::none:
                return isLineSelected;
            case Sketcher::PointPos::start:
                return isStartingPointSelected;
            case Sketcher::PointPos::end:
                return isEndPointSelected;
            case Sketcher::PointPos::mid:
                return isMidPointSelected;
            default:
                return false;
        }
    }

    bool isGeometryPreselected(Sketcher::PointPos pos) const
    {
        switch (pos) {
            case Sketcher::PointPos::none:
                return hovered == SubElementType::edge;
            case Sketcher::PointPos::start:
                return hovered == SubElementType::start;
            case Sketcher::PointPos::end:
                return hovered == SubElementType::end;
            case Sketcher::PointPos::mid:
                return hovered == SubElementType::mid;
            default:
                return false;
        }
    }

    Sketcher::SketchObject* getSketchObject() const
    {
        return sketchView->getSketchObject();
    }

    int ElementNbr;
    int StartingVertex;
    int MidVertex;
    int EndVertex;

    Base::Type GeometryType;
    GeometryState State;

    bool isLineSelected;
    bool isStartingPointSelected;
    bool isEndPointSelected;
    bool isMidPointSelected;


    SubElementType clickedOn;
    SubElementType hovered;
    bool rightClicked;

    QString label;

private:
    ViewProviderSketch* sketchView;
};
// clang-format off

class ElementFilterList: public QListWidget
{
    Q_OBJECT

public:
    explicit ElementFilterList(QWidget* parent = nullptr);
    ~ElementFilterList() override;

protected:
    void changeEvent(QEvent* e) override;
    void languageChange();

private:
    using filterItemRepr =
        std::pair<const char*, const int>;// {filter item text, filter item level}
    inline static const std::vector<filterItemRepr> filterItems = {
        {QT_TR_NOOP("Normal"), 0},
        {QT_TR_NOOP("Construction"), 0},
        {QT_TR_NOOP("Internal"), 0},
        {QT_TR_NOOP("External"), 0},
        {QT_TR_NOOP("All types"), 0},
        {QT_TR_NOOP("Point"), 1},
        {QT_TR_NOOP("Line"), 1},
        {QT_TR_NOOP("Circle"), 1},
        {QT_TR_NOOP("Ellipse"), 1},
        {QT_TR_NOOP("Arc of circle"), 1},
        {QT_TR_NOOP("Arc of ellipse"), 1},
        {QT_TR_NOOP("Arc of hyperbola"), 1},
        {QT_TR_NOOP("Arc of parabola"), 1},
        {QT_TR_NOOP("B-spline"), 1}};
};
}// namespace SketcherGui

class ElementWidgetIcons
{

private:
    ElementWidgetIcons()
    {
        initIcons();
    }

public:
    ElementWidgetIcons(const ElementWidgetIcons&) = delete;
    ElementWidgetIcons(ElementWidgetIcons&&) = delete;
    ElementWidgetIcons& operator=(const ElementWidgetIcons&) = delete;
    ElementWidgetIcons& operator=(ElementWidgetIcons&&) = delete;

    static const QIcon&
    getIcon(Base::Type type, Sketcher::PointPos pos,
            ElementItem::GeometryState icontype = ElementItem::GeometryState::Normal)
    {
        static ElementWidgetIcons elementicons;

        return elementicons.getIconImpl(type, pos, icontype);
    }

private:
    void initIcons()
    {

        icons.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(Part::GeomArcOfCircle::getClassTypeId()),
            std::forward_as_tuple(
                std::initializer_list<
                    std::pair<const Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>> {
                    {Sketcher::PointPos::none, getMultIcon("Sketcher_Element_Arc_Edge")},
                    {Sketcher::PointPos::start, getMultIcon("Sketcher_Element_Arc_StartingPoint")},
                    {Sketcher::PointPos::end, getMultIcon("Sketcher_Element_Arc_EndPoint")},
                    {Sketcher::PointPos::mid, getMultIcon("Sketcher_Element_Arc_MidPoint")}}));

        icons.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(Part::GeomCircle::getClassTypeId()),
            std::forward_as_tuple(
                std::initializer_list<
                    std::pair<const Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>> {
                    {Sketcher::PointPos::none, getMultIcon("Sketcher_Element_Circle_Edge")},
                    {Sketcher::PointPos::mid, getMultIcon("Sketcher_Element_Circle_MidPoint")},
                }));

        icons.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(Part::GeomLineSegment::getClassTypeId()),
            std::forward_as_tuple(
                std::initializer_list<
                    std::pair<const Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>> {
                    {Sketcher::PointPos::none, getMultIcon("Sketcher_Element_Line_Edge")},
                    {Sketcher::PointPos::start, getMultIcon("Sketcher_Element_Line_StartingPoint")},
                    {Sketcher::PointPos::end, getMultIcon("Sketcher_Element_Line_EndPoint")},
                }));

        icons.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(Part::GeomPoint::getClassTypeId()),
            std::forward_as_tuple(
                std::initializer_list<
                    std::pair<const Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>> {
                    {Sketcher::PointPos::start,
                     getMultIcon("Sketcher_Element_Point_StartingPoint")},
                }));

        icons.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(Part::GeomEllipse::getClassTypeId()),
            std::forward_as_tuple(
                std::initializer_list<
                    std::pair<const Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>> {
                    {Sketcher::PointPos::none, getMultIcon("Sketcher_Element_Ellipse_Edge_2")},
                    {Sketcher::PointPos::mid, getMultIcon("Sketcher_Element_Ellipse_CentrePoint")},
                }));

        icons.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(Part::GeomArcOfEllipse::getClassTypeId()),
            std::forward_as_tuple(
                std::initializer_list<
                    std::pair<const Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>> {
                    {Sketcher::PointPos::none, getMultIcon("Sketcher_Element_Elliptical_Arc_Edge")},
                    {Sketcher::PointPos::start,
                     getMultIcon("Sketcher_Element_Elliptical_Arc_Start_Point")},
                    {Sketcher::PointPos::end,
                     getMultIcon("Sketcher_Element_Elliptical_Arc_End_Point")},
                    {Sketcher::PointPos::mid,
                     getMultIcon("Sketcher_Element_Elliptical_Arc_Centre_Point")},
                }));

        icons.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(Part::GeomArcOfHyperbola::getClassTypeId()),
            std::forward_as_tuple(
                std::initializer_list<
                    std::pair<const Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>> {
                    {Sketcher::PointPos::none, getMultIcon("Sketcher_Element_Hyperbolic_Arc_Edge")},
                    {Sketcher::PointPos::start,
                     getMultIcon("Sketcher_Element_Hyperbolic_Arc_Start_Point")},
                    {Sketcher::PointPos::end,
                     getMultIcon("Sketcher_Element_Hyperbolic_Arc_End_Point")},
                    {Sketcher::PointPos::mid,
                     getMultIcon("Sketcher_Element_Hyperbolic_Arc_Centre_Point")},
                }));

        icons.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(Part::GeomArcOfParabola::getClassTypeId()),
            std::forward_as_tuple(
                std::initializer_list<
                    std::pair<const Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>> {
                    {Sketcher::PointPos::none, getMultIcon("Sketcher_Element_Parabolic_Arc_Edge")},
                    {Sketcher::PointPos::start,
                     getMultIcon("Sketcher_Element_Parabolic_Arc_Start_Point")},
                    {Sketcher::PointPos::end,
                     getMultIcon("Sketcher_Element_Parabolic_Arc_End_Point")},
                    {Sketcher::PointPos::mid,
                     getMultIcon("Sketcher_Element_Parabolic_Arc_Centre_Point")},
                }));

        icons.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(Part::GeomBSplineCurve::getClassTypeId()),
            std::forward_as_tuple(
                std::initializer_list<
                    std::pair<const Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>> {
                    {Sketcher::PointPos::none, getMultIcon("Sketcher_Element_BSpline_Edge")},
                    {Sketcher::PointPos::start, getMultIcon("Sketcher_Element_BSpline_StartPoint")},
                    {Sketcher::PointPos::end, getMultIcon("Sketcher_Element_BSpline_EndPoint")},
                }));

        icons.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(Base::Type::BadType),
            std::forward_as_tuple(
                std::initializer_list<
                    std::pair<const Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>> {
                    {Sketcher::PointPos::none,
                     getMultIcon("Sketcher_Element_SelectionTypeInvalid")},
                }));
    }

    const QIcon& getIconImpl(Base::Type type, Sketcher::PointPos pos,
                             ElementItem::GeometryState icontype)
    {

        auto typekey = icons.find(type);

        if (typekey == icons.end()) {// Not supported Geometry Type - Defaults to invalid icon
            typekey = icons.find(Base::Type::BadType);
            pos = Sketcher::PointPos::none;
        }

        auto poskey = typekey->second.find(pos);

        if (poskey == typekey->second.end()) {// invalid PointPos for type - Provide Invalid icon
            typekey = icons.find(Base::Type::BadType);
            pos = Sketcher::PointPos::none;
            poskey = typekey->second.find(pos);
        }

        if (icontype == ElementItem::GeometryState::Normal)
            return std::get<0>(poskey->second);
        else if (icontype == ElementItem::GeometryState::Construction)
            return std::get<1>(poskey->second);
        else if (icontype == ElementItem::GeometryState::External)
            return std::get<2>(poskey->second);
        else// internal alignment
            return std::get<3>(poskey->second);

        // We should never arrive here, as badtype, PointPos::none must exist.
        throw Base::ValueError("Icon for Invalid is missing!!");
    }

    std::tuple<QIcon, QIcon, QIcon, QIcon> getMultIcon(const char* name)
    {
        int hue, sat, val, alp;
        QIcon Normal = Gui::BitmapFactory().iconFromTheme(name);
        QImage imgConstr(Normal.pixmap(std::as_const(Normal).availableSizes()[0]).toImage());
        QImage imgExt(imgConstr);
        QImage imgInt(imgConstr);

        // Create construction/external/internal icons by changing colors.
        for (int ix = 0; ix < imgConstr.width(); ix++) {
            for (int iy = 0; iy < imgConstr.height(); iy++) {
                QColor clr(imgConstr.pixelColor(ix, iy));
                clr.getHsv(&hue, &sat, &val, &alp);
                if (alp > 127 && hue >= 0) {
                    if (sat > 127 && (hue > 330 || hue < 30)) {// change the color of red points.
                        clr.setHsv((hue + 240) % 360, sat, val, alp);
                        imgConstr.setPixelColor(ix, iy, clr);
                        clr.setHsv((hue + 300) % 360, sat, val, alp);
                        imgExt.setPixelColor(ix, iy, clr);
                        clr.setHsv((hue + 60) % 360,
                                   (int)(sat / 3),
                                   std::min((int)(val * 8 / 7), 255),
                                   alp);
                        imgInt.setPixelColor(ix, iy, clr);
                    }
                    else if (sat < 64 && val > 192) {// change the color of white edges.
                        clr.setHsv(240, (255 - sat), val, alp);
                        imgConstr.setPixel(ix, iy, clr.rgba());
                        clr.setHsv(300, (255 - sat), val, alp);
                        imgExt.setPixel(ix, iy, clr.rgba());
                        clr.setHsv(60, (int)(255 - sat) / 2, val, alp);
                        imgInt.setPixel(ix, iy, clr.rgba());
                    }
                }
            }
        }
        QIcon Construction = QIcon(QPixmap::fromImage(imgConstr));
        QIcon External = QIcon(QPixmap::fromImage(imgExt));
        QIcon Internal = QIcon(QPixmap::fromImage(imgInt));

        return std::make_tuple(Normal, Construction, External, Internal);
    }

private:
    std::map<Base::Type, std::map<Sketcher::PointPos, std::tuple<QIcon, QIcon, QIcon, QIcon>>>
        icons;
};

ElementView::ElementView(QWidget* parent)
    : QListWidget(parent)
{
    auto* elementItemDelegate = new ElementItemDelegate(this);
    setItemDelegate(elementItemDelegate);

    QObject::connect(
        elementItemDelegate, &ElementItemDelegate::itemHovered, this, &ElementView::onIndexHovered);

    QObject::connect(
        elementItemDelegate, &ElementItemDelegate::itemChecked, this, &ElementView::onIndexChecked);
}

ElementView::~ElementView()
{}

void ElementView::changeLayer(int layer)
{
    App::Document* doc = App::GetApplication().getActiveDocument();

    if (!doc)
        return;

    doc->openTransaction("Geometry Layer Change");
    std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx(doc->getName());
    for (std::vector<Gui::SelectionObject>::iterator ft = sel.begin(); ft != sel.end(); ++ft) {
        auto sketchobject = ft->getObject<Sketcher::SketchObject>();

        auto geoids = getGeoIdsOfEdgesFromNames(sketchobject, ft->getSubNames());

        auto geometry = sketchobject->Geometry.getValues();
        auto newgeometry(geometry);

        bool anychanged = false;
        for (auto geoid : geoids) {
            if (geoid
                >= 0) {// currently only internal geometry can be changed from one layer to another
                auto currentlayer = getSafeGeomLayerId(geometry[geoid]);
                if (currentlayer != layer) {
                    auto geo = geometry[geoid]->clone();
                    setSafeGeomLayerId(geo, layer);
                    newgeometry[geoid] = geo;
                    anychanged = true;
                }
            }
            else {
                Gui::TranslatedUserWarning(
                    sketchobject,
                    QObject::tr("Unsupported visual layer operation"),
                    QObject::tr("It is currently unsupported to move external geometry to another "
                                "visual layer. External geometry will be omitted"));
            }
        }

        if (anychanged) {
            sketchobject->Geometry.setValues(std::move(newgeometry));
            sketchobject->solve();
        }
    }
    doc->commitTransaction();
}

void ElementView::changeLayer(ElementItem* item, int layer)
{
    App::Document* doc = App::GetApplication().getActiveDocument();

    if (!doc) {
        return;
    }

    doc->openTransaction("Geometry Layer Change");

    auto sketchObject = item->getSketchObject();

    auto geometry = sketchObject->Geometry.getValues();
    auto newGeometry(geometry);

    auto geoid = item->ElementNbr;

    // currently only internal geometry can be changed from one layer to another
    if (geoid >= 0) {
        auto currentLayer = getSafeGeomLayerId(geometry[geoid]);

        if (currentLayer != layer) {
            auto geo = geometry[geoid]->clone();
            setSafeGeomLayerId(geo, layer);
            newGeometry[geoid] = geo;

            sketchObject->Geometry.setValues(std::move(newGeometry));
            sketchObject->solve();
        }
    }
    else {
        Gui::TranslatedUserWarning(
            sketchObject,
            QObject::tr("Unsupported visual layer operation"),
            QObject::tr("It is currently unsupported to move external geometry to another "
                        "visual layer. External geometry will be omitted"));
    }

    doc->commitTransaction();
}

void ElementView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu;
    QList<QListWidgetItem*> items = selectedItems();

    // NOTE: If extending this context menu, be sure to add the items to the translation block at
    // the top of this file

    // CONTEXT_ITEM(ICONSTR,NAMESTR,CMDSTR,FUNC,ACTSONSELECTION)
    CONTEXT_ITEM("Constraint_PointOnPoint",
                 "Point coincidence",
                 "Sketcher_ConstrainCoincident",
                 doPointCoincidence,
                 true)
    CONTEXT_ITEM("Constraint_PointOnObject",
                 "Point on object",
                 "Sketcher_ConstrainPointOnObject",
                 doPointOnObjectConstraint,
                 true)
    CONTEXT_ITEM("Constraint_Horizontal",
                 "Horizontal constraint",
                 "Sketcher_ConstrainHorizontal",
                 doHorizontalConstraint,
                 true)
    CONTEXT_ITEM("Constraint_Vertical",
                 "Vertical constraint",
                 "Sketcher_ConstrainVertical",
                 doVerticalConstraint,
                 true)
    CONTEXT_ITEM("Constraint_Parallel",
                 "Parallel constraint",
                 "Sketcher_ConstrainParallel",
                 doParallelConstraint,
                 true)
    CONTEXT_ITEM("Constraint_Perpendicular",
                 "Perpendicular constraint",
                 "Sketcher_ConstrainPerpendicular",
                 doPerpendicularConstraint,
                 true)
    CONTEXT_ITEM("Constraint_Tangent",
                 "Tangent constraint",
                 "Sketcher_ConstrainTangent",
                 doTangentConstraint,
                 true)
    CONTEXT_ITEM("Constraint_EqualLength",
                 "Equal constraint",
                 "Sketcher_ConstrainEqual",
                 doEqualConstraint,
                 true)
    CONTEXT_ITEM("Constraint_Symmetric",
                 "Symmetric constraint",
                 "Sketcher_ConstrainSymmetric",
                 doSymmetricConstraint,
                 true)
    CONTEXT_ITEM(
        "Constraint_Block", "Block constraint", "Sketcher_ConstrainBlock", doBlockConstraint, true)

    CONTEXT_ITEM("Constraint_HorizontalDistance",
                 "Horizontal dimension",
                 "Sketcher_ConstrainDistanceX",
                 doHorizontalDistance,
                 true)
    CONTEXT_ITEM("Constraint_VerticalDistance",
                 "Vertical dimension",
                 "Sketcher_ConstrainDistanceY",
                 doVerticalDistance,
                 true)
    CONTEXT_ITEM("Constraint_Length",
                 "Length dimension",
                 "Sketcher_ConstrainDistance",
                 doLengthConstraint,
                 true)
    CONTEXT_ITEM("Constraint_Radiam",
                 "Radius or diameter",
                 "Sketcher_ConstrainRadiam",
                 doRadiamConstraint,
                 true)
    CONTEXT_ITEM("Constraint_Radius",
                 "Radius",
                 "Sketcher_ConstrainRadius",
                 doRadiusConstraint,
                 true)
    CONTEXT_ITEM("Constraint_Diameter",
                 "Diameter",
                 "Sketcher_ConstrainDiameter",
                 doDiameterConstraint,
                 true)
    CONTEXT_ITEM("Constraint_InternalAngle",
                 "Angle",
                 "Sketcher_ConstrainAngle",
                 doAngleConstraint,
                 true)
    CONTEXT_ITEM(
        "Constraint_Lock", "Lock", "Sketcher_ConstrainLock", doLockConstraint, true)

    menu.addSeparator();

    CONTEXT_ITEM("Sketcher_ToggleConstruction",
                 "Toggle construction geometry",
                 "Sketcher_ToggleConstruction",
                 doToggleConstruction,
                 true)

    menu.addSeparator();

    CONTEXT_ITEM("Sketcher_SelectConstraints",
                 "Select constraints",
                 "Sketcher_SelectConstraints",
                 doSelectConstraints,
                 true)
    CONTEXT_ITEM(
        "Sketcher_SelectOrigin", "Select Origin", "Sketcher_SelectOrigin", doSelectOrigin, false)
    CONTEXT_ITEM("Sketcher_SelectHorizontalAxis",
                 "Select horizontal axis",
                 "Sketcher_SelectHorizontalAxis",
                 doSelectHAxis,
                 false)
    CONTEXT_ITEM("Sketcher_SelectVerticalAxis",
                 "Select vertical axis",
                 "Sketcher_SelectVerticalAxis",
                 doSelectVAxis,
                 false)

    menu.addSeparator();

    auto submenu = menu.addMenu(tr("Layer"));

    auto addLayerAction = [submenu, this, items](auto&& name, int layernumber) {
        auto action = submenu->addAction(std::forward<decltype(name)>(name), [this, layernumber]() {
            changeLayer(layernumber);
        });
        action->setEnabled(!items.isEmpty());
        return action;
    };

    addLayerAction(tr("Layer 0"), 0);
    addLayerAction(tr("Layer 1"), 1);
    addLayerAction(tr("Hidden"), 2);


    menu.addSeparator();

    QAction* remove = menu.addAction(tr("Delete"), this, &ElementView::deleteSelectedItems);
    remove->setShortcut(QKeySequence(QKeySequence::Delete));
    remove->setEnabled(!items.isEmpty());

    menu.menuAction()->setIconVisibleInMenu(true);

    menu.exec(event->globalPos());
}

CONTEXT_MEMBER_DEF("Sketcher_ConstrainCoincident", doPointCoincidence)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainPointOnObject", doPointOnObjectConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainHorizontal", doHorizontalConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainVertical", doVerticalConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainParallel", doParallelConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainPerpendicular", doPerpendicularConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainTangent", doTangentConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainEqual", doEqualConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainSymmetric", doSymmetricConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainBlock", doBlockConstraint)

CONTEXT_MEMBER_DEF("Sketcher_ConstrainDistanceX", doHorizontalDistance)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainDistanceY", doVerticalDistance)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainDistance", doLengthConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainRadiam", doRadiamConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainRadius", doRadiusConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainDiameter", doDiameterConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainAngle", doAngleConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainLock", doLockConstraint)

CONTEXT_MEMBER_DEF("Sketcher_ToggleConstruction", doToggleConstruction)

CONTEXT_MEMBER_DEF("Sketcher_SelectConstraints", doSelectConstraints)
CONTEXT_MEMBER_DEF("Sketcher_SelectOrigin", doSelectOrigin)
CONTEXT_MEMBER_DEF("Sketcher_SelectHorizontalAxis", doSelectHAxis)
CONTEXT_MEMBER_DEF("Sketcher_SelectVerticalAxis", doSelectVAxis)

void ElementView::deleteSelectedItems()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc)
        return;

    doc->openTransaction("Delete element");
    std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx(doc->getName());
    for (std::vector<Gui::SelectionObject>::iterator ft = sel.begin(); ft != sel.end(); ++ft) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(ft->getObject());
        if (vp) {
            vp->onDelete(ft->getSubNames());
        }
    }
    doc->commitTransaction();
}

void ElementView::onIndexHovered(QModelIndex index)
{
    update(index);

    Q_EMIT onItemHovered(itemFromIndex(index));
}

void ElementView::onIndexChecked(QModelIndex index, Qt::CheckState state)
{
    auto item = itemFromIndex(index);

    changeLayer(item, static_cast<int>(state == Qt::Checked ? ElementItem::Layer::Default : ElementItem::Layer::Hidden));
}

ElementItem* ElementView::itemFromIndex(const QModelIndex& index)
{
    return static_cast<ElementItem*>(QListWidget::itemFromIndex(index));
}

// clang-format on
/* ElementItem delegate ---------------------------------------------------- */
ElementItemDelegate::ElementItemDelegate(ElementView* parent)
    : QStyledItemDelegate(parent)
{  // This class relies on the parent being an ElementView, see getElementtItem
}

void ElementItemDelegate::paint(QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    ElementItem* item = getElementItem(index);

    if (!item) {
        return;
    }

    auto style = option.widget ? option.widget->style() : QApplication::style();

    QStyleOptionViewItem itemOption = option;

    initStyleOption(&itemOption, index);

    if (item->isLineSelected || item->isStartingPointSelected || item->isEndPointSelected
        || item->isMidPointSelected) {
        itemOption.state |= QStyle::State_Active;
    }

    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &itemOption, painter, option.widget);

    drawSubControl(SubControl::CheckBox, painter, option, index);
    drawSubControl(SubControl::LineSelect, painter, option, index);
    drawSubControl(SubControl::StartSelect, painter, option, index);
    drawSubControl(SubControl::EndSelect, painter, option, index);
    drawSubControl(SubControl::MidSelect, painter, option, index);
    drawSubControl(SubControl::Label, painter, option, index);
}

QRect ElementItemDelegate::subControlRect(SubControl element,
                                          const QStyleOptionViewItem& option,
                                          const QModelIndex& index) const
{
    auto itemOption = option;

    auto style = option.widget ? option.widget->style() : QApplication::style();

    initStyleOption(&itemOption, index);

    QRect checkBoxRect =
        style->subElementRect(QStyle::SE_CheckBoxIndicator, &itemOption, option.widget);

    checkBoxRect.moveTo(gap,
                        option.rect.top() + (option.rect.height() - checkBoxRect.height()) / 2);

    if (element == SubControl::CheckBox) {
        return checkBoxRect;
    }

    QRect selectRect =
        style->subElementRect(QStyle::SE_ItemViewItemDecoration, &itemOption, option.widget)
            .translated(checkBoxRect.right() + gap, 0);

    unsigned pos = element - SubControl::LineSelect;

    auto rect = selectRect.translated((selectRect.width() + gap) * pos, 0);

    if (element != SubControl::Label) {
        return rect;
    }

    rect.setRight(itemOption.rect.right());

    return rect;
}

void ElementItemDelegate::drawSubControl(SubControl element,
                                         QPainter* painter,
                                         const QStyleOptionViewItem& option,
                                         const QModelIndex& index) const
{
    auto item = getElementItem(index);
    auto style = option.widget ? option.widget->style() : QApplication::style();

    auto rect = subControlRect(element, option, index);

    auto mousePos = option.widget->mapFromGlobal(QCursor::pos());
    auto isHovered = rect.contains(mousePos);

    auto drawSelectIcon = [&](Sketcher::PointPos pos) {
        auto icon = ElementWidgetIcons::getIcon(item->GeometryType, pos, item->State);

        auto isOptionSelected = option.state & QStyle::State_Selected;
        auto isOptionHovered = option.state & QStyle::State_MouseOver;

        // items that user is not interacting with should be fully opaque
        // only if item is partially selected (so only one part of geometry)
        // the rest should be dimmed out
        auto opacity = isOptionHovered || isOptionSelected ? 0.4 : 1.0;

        if (item->isGeometryPreselected(pos)) {
            opacity = 0.8f;
        }

        if (item->isGeometrySelected(pos)) {
            opacity = 1.0f;
        }

        painter->setOpacity(opacity);
        painter->drawPixmap(rect, icon.pixmap(rect.size()));
    };

    painter->save();

    switch (element) {
        case SubControl::CheckBox: {
            QStyleOptionButton checkboxOption;

            checkboxOption.initFrom(option.widget);
            checkboxOption.rect = rect;

            checkboxOption.state.setFlag(QStyle::State_Enabled, item->canBeHidden());

            if (isHovered) {
                checkboxOption.state |= QStyle::State_MouseOver;
            }

            if (item->isVisible()) {
                checkboxOption.state |= QStyle::State_On;
            }
            else {
                checkboxOption.state |= QStyle::State_Off;
            }

            style->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck,
                                 &checkboxOption,
                                 painter,
                                 option.widget);

            break;
        }

        case LineSelect: {
            drawSelectIcon(Sketcher::PointPos::none);
            break;
        }

        case StartSelect: {
            drawSelectIcon(Sketcher::PointPos::start);
            break;
        }

        case EndSelect: {
            drawSelectIcon(Sketcher::PointPos::end);
            break;
        }

        case MidSelect: {
            drawSelectIcon(Sketcher::PointPos::mid);
            break;
        }

        case Label: {
            QRect rect = subControlRect(SubControl::Label, option, index);

            auto labelBoundingBox = painter->fontMetrics().tightBoundingRect(item->label);

            painter->drawText(rect.x(),
                              option.rect.bottom()
                                  - (option.rect.height() - labelBoundingBox.height()) / 2,
                              item->label);

            break;
        }
    }

    painter->restore();
}
// clang-format off

bool ElementItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                      const QStyleOptionViewItem& option, const QModelIndex& index)
{
    auto item = getElementItem(index);

    auto getSubElementType = [&](QPoint pos) {
        if (subControlRect(SubControl::LineSelect, option, index).contains(pos)) {
            return SubElementType::edge;
        } else if (subControlRect(SubControl::StartSelect, option, index).contains(pos)) {
            return SubElementType::start;
        } else if (subControlRect(SubControl::EndSelect, option, index).contains(pos)) {
            return SubElementType::end;
        } else if (subControlRect(SubControl::MidSelect, option, index).contains(pos)) {
            return SubElementType::mid;
        } else {
            // depending on geometry type by default we select either point or edge
            return item->GeometryType == Part::GeomPoint::getClassTypeId() ? SubElementType::start : SubElementType::edge;
        }
    };

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);

        item->clickedOn = getSubElementType(mouseEvent->pos());
        item->rightClicked = mouseEvent->button() == Qt::RightButton;

        if (item->canBeHidden()) {
            QRect checkboxRect = subControlRect(SubControl::CheckBox, option, index);

            if (mouseEvent->button() == Qt::LeftButton && checkboxRect.contains(mouseEvent->pos())) {
                Q_EMIT itemChecked(index, item->isVisible() ? Qt::Unchecked : Qt::Checked);
            }
        }
    }
    else if (event->type() == QEvent::MouseMove) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);

        item->hovered = getSubElementType(mouseEvent->pos());

        Q_EMIT itemHovered(index);
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

ElementItem* ElementItemDelegate::getElementItem(const QModelIndex& index) const
{
    auto* elementView = static_cast<ElementView*>(parent());
    return elementView->itemFromIndex(index);
}

/* Filter element list widget ------------------------------------------------------ */

enum class GeoFilterType
{
    NormalGeos,
    ConstructionGeos,
    InternalGeos,
    ExternalGeos,
    AllGeosTypes,
    PointGeos,
    LineGeos,
    CircleGeos,
    EllipseGeos,
    ArcGeos,
    ArcOfEllipseGeos,
    HyperbolaGeos,
    ParabolaGeos,
    BSplineGeos
};

ElementFilterList::ElementFilterList(QWidget* parent)
    : QListWidget(parent)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    int filterState = hGrp->GetInt(
        "ElementFilterState",
        std::numeric_limits<int>::max());// INT_MAX = 01111111111111111111111111111111 in binary.

    for (auto const& filterItem : filterItems) {
        Q_UNUSED(filterItem);
        auto it = new QListWidgetItem();
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable);

        bool isChecked = static_cast<bool>(filterState & 1);// get the first bit of filterState
        it->setCheckState(isChecked ? Qt::Checked : Qt::Unchecked);
        filterState = filterState >> 1;// shift right to get rid of the used bit.

        addItem(it);
    }
    languageChange();

    // We need to fix the state of 'All' group checkbox in case it is partially checked.
    int indexOfAllTypes = static_cast<int>(GeoFilterType::AllGeosTypes);
    if (item(indexOfAllTypes)->checkState() == Qt::Unchecked) {
        bool allUnchecked = true;
        for (int i = indexOfAllTypes + 1; i < count(); i++) {
            if (item(i)->checkState() == Qt::Checked) {
                allUnchecked = false;
                break;
            }
        }
        if (!allUnchecked)
            item(indexOfAllTypes)->setCheckState(Qt::PartiallyChecked);
    }
}

ElementFilterList::~ElementFilterList()
{}

void ElementFilterList::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        languageChange();
    }
    QWidget::changeEvent(e);
}

void ElementFilterList::languageChange()
{
    assert(static_cast<int>(filterItems.size()) == count());
    int i = 0;
    for (auto const& filterItem : filterItems) {
        auto text = QStringLiteral("  ").repeated(filterItem.second - 1)
            + (filterItem.second > 0 ? QStringLiteral("- ") : QStringLiteral(""))
            + tr(filterItem.first);
        item(i++)->setText(text);
    }
}


/* TRANSLATOR SketcherGui::TaskSketcherElements */

TaskSketcherElements::TaskSketcherElements(ViewProviderSketch* sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("Sketcher_CreateLine"), tr("Elements"), true, nullptr)
    , sketchView(sketchView)
    , ui(new Ui_TaskSketcherElements())
    , focusItemIndex(-1)
    , previouslySelectedItemIndex(-1)
    , previouslyHoveredItemIndex(-1)
    , previouslyHoveredType(SubElementType::none)
    , isNamingBoxChecked(false)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
#ifdef Q_OS_MAC
    QString cmdKey = QStringLiteral("\xe2\x8c\x98");// U+2318
#else
    // translate the text (it's offered by Qt's translation files)
    // but avoid being picked up by lupdate
    const char* ctrlKey = "Ctrl";
    QString cmdKey = QShortcut::tr(ctrlKey);
#endif
    Q_UNUSED(cmdKey)

    ui->listWidgetElements->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->listWidgetElements->setEditTriggers(QListWidget::NoEditTriggers);
    ui->listWidgetElements->setMouseTracking(true);

    createFilterButtonActions();
    createSettingsButtonActions();

    connectSignals();

    this->groupLayout()->addWidget(proxy);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    ui->filterBox->setChecked(hGrp->GetBool("ElementFilterEnabled", true));
    ui->filterButton->setEnabled(ui->filterBox->isChecked());

    slotElementsChanged();
}

TaskSketcherElements::~TaskSketcherElements()
{
    connectionElementsChanged.disconnect();
}

void TaskSketcherElements::connectSignals()
{
    // connecting the needed signals
    QObject::connect(ui->listWidgetElements,
                     &ElementView::itemPressed,
                     this,
                     &TaskSketcherElements::onListWidgetElementsItemPressed);
    QObject::connect(ui->listWidgetElements,
                     &ElementView::itemEntered,
                     this,
                     &TaskSketcherElements::onListWidgetElementsItemEntered);
    QObject::connect(ui->listWidgetElements,
                     &ElementView::onItemHovered,
                     this,
                     &TaskSketcherElements::onListWidgetElementsMouseMoveOnItem);
    QObject::connect(filterList,
                     &QListWidget::itemChanged,
                     this,
                     &TaskSketcherElements::onListMultiFilterItemChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6,7,0)
    QObject::connect(ui->filterBox,
                     &QCheckBox::checkStateChanged,
                     this,
                     &TaskSketcherElements::onFilterBoxStateChanged);
#else
    QObject::connect(ui->filterBox,
                     &QCheckBox::stateChanged,
                     this,
                     &TaskSketcherElements::onFilterBoxStateChanged);
#endif
    QObject::connect(
        ui->settingsButton, &QToolButton::clicked, ui->settingsButton, &QToolButton::showMenu);
    QObject::connect(std::as_const(ui->settingsButton)->actions()[0],
                     &QAction::changed,
                     this,
                     &TaskSketcherElements::onSettingsExtendedInformationChanged);
    QObject::connect(
        ui->filterButton, &QToolButton::clicked, ui->filterButton, &QToolButton::showMenu);

    //NOLINTBEGIN
    connectionElementsChanged = sketchView->signalElementsChanged.connect(
        std::bind(&SketcherGui::TaskSketcherElements::slotElementsChanged, this));
    //NOLINTEND
}

/* filter functions --------------------------------------------------- */

void TaskSketcherElements::createFilterButtonActions()
{
    auto* action = new QWidgetAction(this);
    filterList = new ElementFilterList(this);
    action->setDefaultWidget(filterList);
    std::as_const(ui->filterButton)->addAction(action);
}

void TaskSketcherElements::onFilterBoxStateChanged(int val)
{
    Q_UNUSED(val);
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrp->SetBool("ElementFilterEnabled", ui->filterBox->checkState() == Qt::Checked);

    ui->filterButton->setEnabled(ui->filterBox->checkState() == Qt::Checked);
    slotElementsChanged();
}

void TaskSketcherElements::onListMultiFilterItemChanged(QListWidgetItem* item)
{
    {
        QSignalBlocker sigblk(filterList);

        int index = filterList->row(item);
        int indexOfAllTypes = static_cast<int>(GeoFilterType::AllGeosTypes);

        if (index == indexOfAllTypes) {
            for (int i = indexOfAllTypes + 1; i < filterList->count(); i++) {
                filterList->item(i)->setCheckState(item->checkState());
            }
        }
        else if (index > indexOfAllTypes) {
            bool atLeastOneUnchecked = false;
            bool atLeastOneChecked = false;

            for (int i = indexOfAllTypes + 1; i < filterList->count(); i++) {
                if (filterList->item(i)->checkState() == Qt::Checked)
                    atLeastOneChecked = true;
                if (filterList->item(i)->checkState() == Qt::Unchecked)
                    atLeastOneUnchecked = true;
            }
            if (atLeastOneChecked && atLeastOneUnchecked)
                filterList->item(indexOfAllTypes)->setCheckState(Qt::PartiallyChecked);
            else if (atLeastOneUnchecked)
                filterList->item(indexOfAllTypes)->setCheckState(Qt::Unchecked);
            else if (atLeastOneChecked)
                filterList->item(indexOfAllTypes)->setCheckState(Qt::Checked);
        }
    }

    // Save the state of the filter.
    int filterState = 0; // All bits are cleared.
    for (int i = filterList->count() - 1; i >= 0; i--) {
        bool isChecked = filterList->item(i)->checkState() == Qt::Checked;
        filterState = filterState << 1;// we shift left first, else the list is shifted at the end.
        filterState = filterState | (isChecked ? 1 : 0);
    }
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrp->SetInt("ElementFilterState", filterState);

    updateVisibility();
}

void TaskSketcherElements::setItemVisibility(QListWidgetItem* it)
{
    auto* item = static_cast<ElementItem*>(it);

    if (ui->filterBox->checkState() == Qt::Unchecked) {
        item->setHidden(false);
        return;
    }

    using GeometryState = ElementItem::GeometryState;

    if ((filterList->item(static_cast<int>(GeoFilterType::NormalGeos))->checkState()
             == Qt::Unchecked
         && item->State == GeometryState::Normal)
        || (filterList->item(static_cast<int>(GeoFilterType::ConstructionGeos))->checkState()
                == Qt::Unchecked
            && item->State == GeometryState::Construction)
        || (filterList->item(static_cast<int>(GeoFilterType::InternalGeos))->checkState()
                == Qt::Unchecked
            && item->State == GeometryState::InternalAlignment)
        || (filterList->item(static_cast<int>(GeoFilterType::ExternalGeos))->checkState()
                == Qt::Unchecked
            && item->State == GeometryState::External)
        || (filterList->item(static_cast<int>(GeoFilterType::PointGeos))->checkState()
                == Qt::Unchecked
            && item->GeometryType == Part::GeomPoint::getClassTypeId())
        || (filterList->item(static_cast<int>(GeoFilterType::LineGeos))->checkState()
                == Qt::Unchecked
            && item->GeometryType == Part::GeomLineSegment::getClassTypeId())
        || (filterList->item(static_cast<int>(GeoFilterType::CircleGeos))->checkState()
                == Qt::Unchecked
            && item->GeometryType == Part::GeomCircle::getClassTypeId())
        || (filterList->item(static_cast<int>(GeoFilterType::EllipseGeos))->checkState()
                == Qt::Unchecked
            && item->GeometryType == Part::GeomEllipse::getClassTypeId())
        || (filterList->item(static_cast<int>(GeoFilterType::ArcGeos))->checkState()
                == Qt::Unchecked
            && item->GeometryType == Part::GeomArcOfCircle::getClassTypeId())
        || (filterList->item(static_cast<int>(GeoFilterType::ArcOfEllipseGeos))->checkState()
                == Qt::Unchecked
            && item->GeometryType == Part::GeomArcOfEllipse::getClassTypeId())
        || (filterList->item(static_cast<int>(GeoFilterType::HyperbolaGeos))->checkState()
                == Qt::Unchecked
            && item->GeometryType == Part::GeomArcOfHyperbola::getClassTypeId())
        || (filterList->item(static_cast<int>(GeoFilterType::ParabolaGeos))->checkState()
                == Qt::Unchecked
            && item->GeometryType == Part::GeomArcOfParabola::getClassTypeId())
        || (filterList->item(static_cast<int>(GeoFilterType::BSplineGeos))->checkState()
                == Qt::Unchecked
            && item->GeometryType == Part::GeomBSplineCurve::getClassTypeId())) {
        item->setHidden(true);
        return;
    }
    item->setHidden(false);
}

void TaskSketcherElements::updateVisibility()
{
    for (int i = 0; i < ui->listWidgetElements->count(); i++) {
        setItemVisibility(ui->listWidgetElements->item(i));
    }
}

/*------------------*/
// clang-format on
void TaskSketcherElements::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // update the listwidget
    auto updateListWidget = [this](auto& modified_item) {
        QSignalBlocker sigblk(this->ui->listWidgetElements);
        if (modified_item == nullptr) {
            return;
        }
        bool is_selected = modified_item->isSelected();
        const bool should_be_selected = modified_item->isLineSelected
            || modified_item->isStartingPointSelected || modified_item->isEndPointSelected
            || modified_item->isMidPointSelected;

        // If an element is already selected and a new subelement gets selected
        // (eg., if you select the arc of a circle then select the center as
        // well), the new subelement won't get highlighted in the list until you
        // mouseover the list.  To avoid this, we deselect first to trigger a
        // redraw.
        if (should_be_selected && is_selected) {
            modified_item->setSelected(false);
            is_selected = false;
        }

        if (should_be_selected != is_selected) {
            modified_item->setSelected(should_be_selected);
        }
    };

    switch (msg.Type) {
        case Gui::SelectionChanges::ClrSelection: {
            clearWidget();
            return;
        }
        case Gui::SelectionChanges::AddSelection:
        case Gui::SelectionChanges::RmvSelection: {
            bool select = (msg.Type == Gui::SelectionChanges::AddSelection);
            // is it this object??
            if (strcmp(msg.pDocName, sketchView->getSketchObject()->getDocument()->getName()) != 0
                || strcmp(msg.pObjectName, sketchView->getSketchObject()->getNameInDocument())
                    != 0) {
                return;
            }
            if (!msg.pSubName) {
                return;
            }
            ElementItem* modified_item = nullptr;
            QString expr = QString::fromLatin1(msg.pSubName);
            std::string shapetype(msg.pSubName);
            // if-else edge vertex
            if (shapetype.starts_with("Edge")) {
                QRegularExpression rx(QStringLiteral("^Edge(\\d+)$"));
                QRegularExpressionMatch match;
                boost::ignore_unused(expr.indexOf(rx, 0, &match));
                if (!match.hasMatch()) {
                    return;
                }
                bool ok;
                int ElementId = match.captured(1).toInt(&ok) - 1;
                if (!ok) {
                    return;
                }
                int countItems = ui->listWidgetElements->count();
                // TODO: This and the loop below get slow when we have a lot of items.
                // Perhaps we should also maintain a map so that we can look up items
                // by element number.
                for (int i = 0; i < countItems; i++) {
                    auto* item = static_cast<ElementItem*>(ui->listWidgetElements->item(i));
                    if (item->ElementNbr == ElementId) {
                        item->isLineSelected = select;
                        modified_item = item;
                        SketcherGui::scrollTo(ui->listWidgetElements, i, select);
                        break;
                    }
                }
            }
            else if (shapetype.starts_with("ExternalEdge")) {
                QRegularExpression rx(QStringLiteral("^ExternalEdge(\\d+)$"));
                QRegularExpressionMatch match;
                boost::ignore_unused(expr.indexOf(rx, 0, &match));
                if (!match.hasMatch()) {
                    return;
                }
                bool ok;
                int ElementId = -match.captured(1).toInt(&ok) - 2;
                if (!ok) {
                    return;
                }
                int countItems = ui->listWidgetElements->count();
                for (int i = 0; i < countItems; i++) {
                    auto* item = static_cast<ElementItem*>(ui->listWidgetElements->item(i));
                    if (item->ElementNbr == ElementId) {
                        item->isLineSelected = select;
                        modified_item = item;
                        break;
                    }
                }
            }
            else if (shapetype.starts_with("Vertex")) {
                QRegularExpression rx(QStringLiteral("^Vertex(\\d+)$"));
                QRegularExpressionMatch match;
                boost::ignore_unused(expr.indexOf(rx, 0, &match));
                if (!match.hasMatch()) {
                    return;
                }
                bool ok;
                int ElementId = match.captured(1).toInt(&ok) - 1;
                if (!ok) {
                    return;
                }
                // Get the GeoID&Pos
                int GeoId;
                Sketcher::PointPos PosId;
                sketchView->getSketchObject()->getGeoVertexIndex(ElementId, GeoId, PosId);

                int countItems = ui->listWidgetElements->count();
                for (int i = 0; i < countItems; i++) {
                    auto* item = static_cast<ElementItem*>(ui->listWidgetElements->item(i));
                    if (item->ElementNbr == GeoId) {
                        modified_item = item;
                        switch (PosId) {
                            case Sketcher::PointPos::start:
                                item->isStartingPointSelected = select;
                                break;
                            case Sketcher::PointPos::end:
                                item->isEndPointSelected = select;
                                break;
                            case Sketcher::PointPos::mid:
                                item->isMidPointSelected = select;
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                }
            }
            updateListWidget(modified_item);
        }
        default:
            return;
    }
}
// clang-format off

void TaskSketcherElements::onListWidgetElementsItemPressed(QListWidgetItem* it)
{
    // We use itemPressed instead of previously used ItemSelectionChanged because if user click on
    // already selected item, ItemSelectionChanged didn't trigger.
    if (!it)
        return;

    auto* itf = static_cast<ElementItem*>(it);
    bool rightClickOnSelected = itf->rightClicked
        && (itf->isLineSelected || itf->isStartingPointSelected || itf->isEndPointSelected
            || itf->isMidPointSelected);
    itf->rightClicked = false;
    if (rightClickOnSelected)// if user right clicked on a selected item, change nothing.
        return;

    {
        QSignalBlocker sigblk(ui->listWidgetElements);

        bool multipleselection = false;
        bool multipleconsecutiveselection = false;
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            multipleselection = true;
        if (QApplication::keyboardModifiers() == Qt::ShiftModifier)
            multipleconsecutiveselection = true;

        if (multipleselection
            && multipleconsecutiveselection) {// ctrl takes priority over shift functionality
            multipleselection = true;
            multipleconsecutiveselection = false;
        }

        std::vector<std::string> elementSubNames;
        std::string doc_name = sketchView->getSketchObject()->getDocument()->getName();
        std::string obj_name = sketchView->getSketchObject()->getNameInDocument();

        bool block = this->blockSelection(true);// avoid to be notified by itself
        Gui::Selection().clearSelection();

        for (int i = 0; i < ui->listWidgetElements->count(); i++) {
            auto* item = static_cast<ElementItem*>(ui->listWidgetElements->item(i));

            if (!multipleselection && !multipleconsecutiveselection) {
                // if not multiple selection, then all are disabled but the one that was just
                // selected
                item->isLineSelected = false;
                item->isStartingPointSelected = false;
                item->isEndPointSelected = false;
                item->isMidPointSelected = false;
            }

            if (item == itf) {
                if (item->clickedOn == SubElementType::mid
                    && (item->GeometryType == Part::GeomArcOfCircle::getClassTypeId()
                        || item->GeometryType == Part::GeomArcOfEllipse::getClassTypeId()
                        || item->GeometryType == Part::GeomArcOfHyperbola::getClassTypeId()
                        || item->GeometryType == Part::GeomArcOfParabola::getClassTypeId()
                        || item->GeometryType == Part::GeomCircle::getClassTypeId()
                        || item->GeometryType == Part::GeomEllipse::getClassTypeId())) {
                    item->isMidPointSelected = !item->isMidPointSelected;
                }
                else if (item->clickedOn == SubElementType::start
                         && (item->GeometryType == Part::GeomPoint::getClassTypeId()
                             || item->GeometryType == Part::GeomArcOfCircle::getClassTypeId()
                             || item->GeometryType == Part::GeomArcOfEllipse::getClassTypeId()
                             || item->GeometryType == Part::GeomArcOfHyperbola::getClassTypeId()
                             || item->GeometryType == Part::GeomArcOfParabola::getClassTypeId()
                             || item->GeometryType == Part::GeomLineSegment::getClassTypeId()
                             || item->GeometryType == Part::GeomBSplineCurve::getClassTypeId())) {
                    item->isStartingPointSelected = !item->isStartingPointSelected;
                }
                else if (item->clickedOn == SubElementType::end
                         && (item->GeometryType == Part::GeomArcOfCircle::getClassTypeId()
                             || item->GeometryType == Part::GeomArcOfEllipse::getClassTypeId()
                             || item->GeometryType == Part::GeomArcOfHyperbola::getClassTypeId()
                             || item->GeometryType == Part::GeomArcOfParabola::getClassTypeId()
                             || item->GeometryType == Part::GeomLineSegment::getClassTypeId()
                             || item->GeometryType == Part::GeomBSplineCurve::getClassTypeId())) {
                    item->isEndPointSelected = !item->isEndPointSelected;
                }
                else if (item->clickedOn == SubElementType::edge
                         && item->GeometryType != Part::GeomPoint::getClassTypeId()) {
                    item->isLineSelected = !item->isLineSelected;
                }
                item->clickedOn = SubElementType::none;
            }
            else if (multipleconsecutiveselection && previouslySelectedItemIndex >= 0
                     && !rightClickOnSelected
                     && ((i > focusItemIndex && i < previouslySelectedItemIndex)
                         || (i < focusItemIndex && i > previouslySelectedItemIndex))) {
                if (item->GeometryType == Part::GeomPoint::getClassTypeId()) {
                    item->isStartingPointSelected = true;
                }
                else {
                    item->isLineSelected = true;
                }
            }

            // first update the listwidget. Item is selected if at least one element of the geo is
            // selected.
            bool selected = item->isLineSelected || item->isStartingPointSelected
                || item->isEndPointSelected || item->isMidPointSelected;

            {
                QSignalBlocker sigblk(ui->listWidgetElements);

                if (item->isSelected() && selected) {
                    item->setSelected(
                        false);// if already selected and changing or adding subelement, ensure
                               // selection change is triggered, which ensures timely repaint
                    item->setSelected(selected);
                }
                else {
                    item->setSelected(selected);
                }
            }

            // now the scene
            std::stringstream ss;

            if (item->isLineSelected) {
                if (item->ElementNbr >= 0) {
                    ss << "Edge" << item->ElementNbr + 1;
                }
                else {
                    ss << "ExternalEdge" << -item->ElementNbr - 2;
                }
                elementSubNames.push_back(ss.str());
            }

            auto selectVertex = [&ss, &elementSubNames](bool subelementselected, int vertexid) {
                if (subelementselected) {
                    int vertex;
                    ss.str(std::string());
                    vertex = vertexid;
                    if (vertex != -1) {
                        ss << "Vertex" << vertex + 1;
                        elementSubNames.push_back(ss.str());
                    }
                }
            };

            selectVertex(item->isStartingPointSelected, item->StartingVertex);
            selectVertex(item->isEndPointSelected, item->EndVertex);
            selectVertex(item->isMidPointSelected, item->MidVertex);
        }

        for (const auto& elementSubName : elementSubNames) {
            Gui::Selection().addSelection2(
                doc_name.c_str(),
                obj_name.c_str(),
                sketchView->getSketchObject()->convertSubName(elementSubName).c_str());
        }

        this->blockSelection(block);
    }

    if (focusItemIndex > -1 && focusItemIndex < ui->listWidgetElements->count())
        previouslySelectedItemIndex = focusItemIndex;

    ui->listWidgetElements->repaint();
}

bool TaskSketcherElements::hasInputWidgetFocused()
{
    QWidget* focusedWidget = QApplication::focusWidget();
    return qobject_cast<QLineEdit*>(focusedWidget) != nullptr;
}

void TaskSketcherElements::onListWidgetElementsItemEntered(QListWidgetItem* item)
{
    if (hasInputWidgetFocused()) {
        return;
    }
    ui->listWidgetElements->setFocus();

    focusItemIndex = ui->listWidgetElements->row(item);
}

void TaskSketcherElements::onListWidgetElementsMouseMoveOnItem(QListWidgetItem* it)
{
    if (hasInputWidgetFocused()) {
        return;
    }

    auto* item = static_cast<ElementItem*>(it);

    if (!item
        || (ui->listWidgetElements->row(item) == previouslyHoveredItemIndex
            && item->hovered == previouslyHoveredType))
        return;

    Gui::Selection().rmvPreselect();

    bool validmid = item->hovered == SubElementType::mid
        && (item->GeometryType == Part::GeomArcOfCircle::getClassTypeId()
            || item->GeometryType == Part::GeomArcOfEllipse::getClassTypeId()
            || item->GeometryType == Part::GeomArcOfHyperbola::getClassTypeId()
            || item->GeometryType == Part::GeomArcOfParabola::getClassTypeId()
            || item->GeometryType == Part::GeomCircle::getClassTypeId()
            || item->GeometryType == Part::GeomEllipse::getClassTypeId());

    bool validstartpoint = item->hovered == SubElementType::start
        && (item->GeometryType == Part::GeomPoint::getClassTypeId()
            || item->GeometryType == Part::GeomArcOfCircle::getClassTypeId()
            || item->GeometryType == Part::GeomArcOfEllipse::getClassTypeId()
            || item->GeometryType == Part::GeomArcOfHyperbola::getClassTypeId()
            || item->GeometryType == Part::GeomArcOfParabola::getClassTypeId()
            || item->GeometryType == Part::GeomLineSegment::getClassTypeId()
            || item->GeometryType == Part::GeomBSplineCurve::getClassTypeId());

    bool validendpoint = item->hovered == SubElementType::end
        && (item->GeometryType == Part::GeomArcOfCircle::getClassTypeId()
            || item->GeometryType == Part::GeomArcOfEllipse::getClassTypeId()
            || item->GeometryType == Part::GeomArcOfHyperbola::getClassTypeId()
            || item->GeometryType == Part::GeomArcOfParabola::getClassTypeId()
            || item->GeometryType == Part::GeomLineSegment::getClassTypeId()
            || item->GeometryType == Part::GeomBSplineCurve::getClassTypeId());

    bool validedge = item->hovered == SubElementType::edge
        && item->GeometryType != Part::GeomPoint::getClassTypeId();

    if (validmid || validstartpoint || validendpoint || validedge) {
        std::string doc_name = sketchView->getSketchObject()->getDocument()->getName();
        std::string obj_name = sketchView->getSketchObject()->getNameInDocument();

        std::stringstream ss;

        auto preselectvertex = [&](int geoid, Sketcher::PointPos pos) {
            int vertex = sketchView->getSketchObject()->getVertexIndexGeoPos(geoid, pos);
            if (vertex != -1) {
                ss << "Vertex" << vertex + 1;
                Gui::Selection().setPreselect(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
            }
        };

        if (item->hovered == SubElementType::start)
            preselectvertex(item->ElementNbr, Sketcher::PointPos::start);
        else if (item->hovered == SubElementType::end)
            preselectvertex(item->ElementNbr, Sketcher::PointPos::end);
        else if (item->hovered == SubElementType::mid)
            preselectvertex(item->ElementNbr, Sketcher::PointPos::mid);
        else if (item->hovered == SubElementType::edge) {
            if (item->ElementNbr >= 0) {
                ss << "Edge" << item->ElementNbr + 1;
            }
            else {
                ss << "ExternalEdge" << -item->ElementNbr - 2;
            }
            Gui::Selection().setPreselect(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
        }
    }

    previouslyHoveredItemIndex = ui->listWidgetElements->row(item);
    previouslyHoveredType = item->hovered;
}

void TaskSketcherElements::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    Gui::Selection().rmvPreselect();
    ui->listWidgetElements->clearFocus();
}

void TaskSketcherElements::slotElementsChanged()
{
    assert(sketchView);
    // Build up ListView with the elements
    Sketcher::SketchObject* sketch = sketchView->getSketchObject();
    const std::vector<Part::Geometry*>& vals = sketch->Geometry.getValues();

    ui->listWidgetElements->clear();

    using GeometryState = ElementItem::GeometryState;

    int i = 1;
    for (std::vector<Part::Geometry*>::const_iterator it = vals.begin(); it != vals.end();
         ++it, ++i) {
        Base::Type type = (*it)->getTypeId();
        GeometryState state = GeometryState::Normal;

        bool construction = Sketcher::GeometryFacade::getConstruction(*it);
        bool internalAligned = Sketcher::GeometryFacade::isInternalAligned(*it);

        auto layerId = getSafeGeomLayerId(*it);

        if (internalAligned)
            state = GeometryState::InternalAlignment;
        else if (construction)// Caution, internalAligned geos are construction too. So the 'if' and
                              // 'else if' cannot be swapped.
            state = GeometryState::Construction;

        auto IdInformation = [this, i, layerId]() {
            if (sketchView->VisualLayerList.getSize() > 1)
                return QStringLiteral("(Edge%1#ID%2#VL%3)").arg(i).arg(i - 1).arg(layerId);
            else
                return QStringLiteral("(Edge%1#ID%2)").arg(i).arg(i - 1);
        };

        auto* itemN = new ElementItem(
            i - 1,
            sketchView->getSketchObject()->getVertexIndexGeoPos(i - 1, Sketcher::PointPos::start),
            sketchView->getSketchObject()->getVertexIndexGeoPos(i - 1, Sketcher::PointPos::mid),
            sketchView->getSketchObject()->getVertexIndexGeoPos(i - 1, Sketcher::PointPos::end),
            type,
            state,
            type == Part::GeomPoint::getClassTypeId()
                ? (isNamingBoxChecked ? (tr("Point") + IdInformation())
                           + (construction
                                  ? (QStringLiteral("-") + tr("Construction"))
                                  : (internalAligned ? (QStringLiteral("-") + tr("Internal"))
                                                     : QStringLiteral("")))
                                      : (QStringLiteral("%1-").arg(i) + tr("Point")))
                : type == Part::GeomLineSegment::getClassTypeId()
                ? (isNamingBoxChecked ? (tr("Line") + IdInformation())
                           + (construction
                                  ? (QStringLiteral("-") + tr("Construction"))
                                  : (internalAligned ? (QStringLiteral("-") + tr("Internal"))
                                                     : QStringLiteral("")))
                                      : (QStringLiteral("%1-").arg(i) + tr("Line")))
                : type == Part::GeomArcOfCircle::getClassTypeId()
                ? (isNamingBoxChecked ? (tr("Arc") + IdInformation())
                           + (construction
                                  ? (QStringLiteral("-") + tr("Construction"))
                                  : (internalAligned ? (QStringLiteral("-") + tr("Internal"))
                                                     : QStringLiteral("")))
                                      : (QStringLiteral("%1-").arg(i) + tr("Arc")))
                : type == Part::GeomCircle::getClassTypeId()
                ? (isNamingBoxChecked ? (tr("Circle") + IdInformation())
                           + (construction
                                  ? (QStringLiteral("-") + tr("Construction"))
                                  : (internalAligned ? (QStringLiteral("-") + tr("Internal"))
                                                     : QStringLiteral("")))
                                      : (QStringLiteral("%1-").arg(i) + tr("Circle")))
                : type == Part::GeomEllipse::getClassTypeId()
                ? (isNamingBoxChecked ? (tr("Ellipse") + IdInformation())
                           + (construction
                                  ? (QStringLiteral("-") + tr("Construction"))
                                  : (internalAligned ? (QStringLiteral("-") + tr("Internal"))
                                                     : QStringLiteral("")))
                                      : (QStringLiteral("%1-").arg(i) + tr("Ellipse")))
                : type == Part::GeomArcOfEllipse::getClassTypeId()
                ? (isNamingBoxChecked ? (tr("Elliptical Arc") + IdInformation())
                           + (construction
                                  ? (QStringLiteral("-") + tr("Construction"))
                                  : (internalAligned ? (QStringLiteral("-") + tr("Internal"))
                                                     : QStringLiteral("")))
                                      : (QStringLiteral("%1-").arg(i) + tr("Elliptical arc")))
                : type == Part::GeomArcOfHyperbola::getClassTypeId()
                ? (isNamingBoxChecked ? (tr("Hyperbolic Arc") + IdInformation())
                           + (construction
                                  ? (QStringLiteral("-") + tr("Construction"))
                                  : (internalAligned ? (QStringLiteral("-") + tr("Internal"))
                                                     : QStringLiteral("")))
                                      : (QStringLiteral("%1-").arg(i) + tr("Hyperbolic arc")))
                : type == Part::GeomArcOfParabola::getClassTypeId()
                ? (isNamingBoxChecked ? (tr("Parabolic Arc") + IdInformation())
                           + (construction
                                  ? (QStringLiteral("-") + tr("Construction"))
                                  : (internalAligned ? (QStringLiteral("-") + tr("Internal"))
                                                     : QStringLiteral("")))
                                      : (QStringLiteral("%1-").arg(i) + tr("Parabolic arc")))
                : type == Part::GeomBSplineCurve::getClassTypeId()
                ? (isNamingBoxChecked ? (tr("B-spline") + IdInformation())
                           + (construction
                                  ? (QStringLiteral("-") + tr("Construction"))
                                  : (internalAligned ? (QStringLiteral("-") + tr("Internal"))
                                                     : QStringLiteral("")))
                                      : (QStringLiteral("%1-").arg(i) + tr("B-spline")))
                : (isNamingBoxChecked ? (tr("Other") + IdInformation())
                           + (construction
                                  ? (QStringLiteral("-") + tr("Construction"))
                                  : (internalAligned ? (QStringLiteral("-") + tr("Internal"))
                                                     : QStringLiteral("")))
                                      : (QStringLiteral("%1-").arg(i) + tr("Other"))),
            sketchView);

        ui->listWidgetElements->addItem(itemN);

        setItemVisibility(itemN);
    }

    const std::vector<Part::Geometry*>& ext_vals =
        sketchView->getSketchObject()->getExternalGeometry();

    const std::vector<App::DocumentObject*> linkobjs =
        sketchView->getSketchObject()->ExternalGeometry.getValues();
    const std::vector<std::string> linksubs =
        sketchView->getSketchObject()->ExternalGeometry.getSubValues();

    int j = 1;
    for (std::vector<Part::Geometry*>::const_iterator it = ext_vals.begin(); it != ext_vals.end();
         ++it, ++i, ++j) {
        Base::Type type = (*it)->getTypeId();

        if (j > 2) {// we do not want the H and V axes

            auto layerId = getSafeGeomLayerId(*it);

            auto IdInformation = [this, j, layerId](bool link) {
                if (sketchView->VisualLayerList.getSize() > 1) {
                    if (link) {
                        return QStringLiteral("(ExternalEdge%1#ID%2#VL%3, ")
                            .arg(j - 2)
                            .arg(-j)
                            .arg(layerId);
                    }
                    else {
                        return QStringLiteral("(ExternalEdge%1#ID%2#VL%3)")
                            .arg(j - 2)
                            .arg(-j)
                            .arg(layerId);
                    }
                }
                else {
                    if (link) {
                        return QStringLiteral("(ExternalEdge%1#ID%2, ").arg(j - 2).arg(-j);
                    }
                    else {
                        return QStringLiteral("(ExternalEdge%1#ID%2)").arg(j - 2).arg(-j);
                    }
                }
            };

            QString linkname;

            if (isNamingBoxChecked) {
                if (size_t(j - 3) < linkobjs.size() && size_t(j - 3) < linksubs.size()) {
                    linkname = IdInformation(true)
                        + QString::fromUtf8(linkobjs[j - 3]->getNameInDocument())
                        + QStringLiteral(".") + QString::fromUtf8(linksubs[j - 3].c_str())
                        + QStringLiteral(")");
                }
                else {
                    linkname = IdInformation(false);
                }
            }

            GeometryState state = GeometryState::External;

            auto* itemN = new ElementItem(
                -j,
                sketchView->getSketchObject()->getVertexIndexGeoPos(-j, Sketcher::PointPos::start),
                sketchView->getSketchObject()->getVertexIndexGeoPos(-j, Sketcher::PointPos::mid),
                sketchView->getSketchObject()->getVertexIndexGeoPos(-j, Sketcher::PointPos::end),
                type,
                state,
                type == Part::GeomPoint::getClassTypeId()
                    ? (isNamingBoxChecked ? (tr("Point") + linkname)
                                          : (QStringLiteral("%1-").arg(i - 2) + tr("Point")))
                    : type == Part::GeomLineSegment::getClassTypeId()
                    ? (isNamingBoxChecked ? (tr("Line") + linkname)
                                          : (QStringLiteral("%1-").arg(i - 2) + tr("Line")))
                    : type == Part::GeomArcOfCircle::getClassTypeId()
                    ? (isNamingBoxChecked ? (tr("Arc") + linkname)
                                          : (QStringLiteral("%1-").arg(i - 2) + tr("Arc")))
                    : type == Part::GeomCircle::getClassTypeId()
                    ? (isNamingBoxChecked ? (tr("Circle") + linkname)
                                          : (QStringLiteral("%1-").arg(i - 2) + tr("Circle")))
                    : type == Part::GeomEllipse::getClassTypeId()
                    ? (isNamingBoxChecked ? (tr("Ellipse") + linkname)
                                          : (QStringLiteral("%1-").arg(i - 2) + tr("Ellipse")))
                    : type == Part::GeomArcOfEllipse::getClassTypeId()
                    ? (isNamingBoxChecked
                           ? (tr("Elliptical Arc") + linkname)
                           : (QStringLiteral("%1-").arg(i - 2) + tr("Elliptical arc")))
                    : type == Part::GeomArcOfHyperbola::getClassTypeId()
                    ? (isNamingBoxChecked
                           ? (tr("Hyperbolic Arc") + linkname)
                           : (QStringLiteral("%1-").arg(i - 2) + tr("Hyperbolic arc")))
                    : type == Part::GeomArcOfParabola::getClassTypeId()
                    ? (isNamingBoxChecked
                           ? (tr("Parabolic Arc") + linkname)
                           : (QStringLiteral("%1-").arg(i - 2) + tr("Parabolic arc")))
                    : type == Part::GeomBSplineCurve::getClassTypeId()
                    ? (isNamingBoxChecked ? (tr("B-spline") + linkname)
                                          : (QStringLiteral("%1-").arg(i - 2) + tr("B-spline")))
                    : (isNamingBoxChecked ? (tr("Other") + linkname)
                                          : (QStringLiteral("%1-").arg(i - 2) + tr("Other"))),
                sketchView);

            ui->listWidgetElements->addItem(itemN);

            setItemVisibility(itemN);
        }
    }
}

void TaskSketcherElements::clearWidget()
{
    {
        QSignalBlocker sigblk(ui->listWidgetElements);
        ui->listWidgetElements->clearSelection();
    }

    // update widget
    int countItems = ui->listWidgetElements->count();
    for (int i = 0; i < countItems; i++) {
        auto* item = static_cast<ElementItem*>(ui->listWidgetElements->item(i));

        item->isLineSelected = false;
        item->isStartingPointSelected = false;
        item->isEndPointSelected = false;
        item->isMidPointSelected = false;
    }
}

void TaskSketcherElements::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

/* Settings menu ==================================================*/
void TaskSketcherElements::createSettingsButtonActions()
{
    auto* action = new QAction(tr("Extended information"), this);

    action->setCheckable(true);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/Elements");
    {
        QSignalBlocker block(this);
        action->setChecked(hGrp->GetBool("ExtendedNaming", false));
    }

    ui->settingsButton->addAction(action);

    isNamingBoxChecked = hGrp->GetBool("ExtendedNaming", false);
}

void TaskSketcherElements::onSettingsExtendedInformationChanged()
{
    QList<QAction*> acts = ui->settingsButton->actions();
    isNamingBoxChecked = acts[0]->isChecked();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/Elements");
    hGrp->SetBool("ExtendedNaming", isNamingBoxChecked);

    slotElementsChanged();
}

#include "TaskSketcherElements.moc"// For Delegate as it is QOBJECT
#include "moc_TaskSketcherElements.cpp"
// clang-format on
