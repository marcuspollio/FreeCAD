# ***************************************************************************
# *   Copyright (c) 2016 Qingfeng Xia <qingfeng.xia()eng.ox.ac.uk>          *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM result mechanical document object"
__author__ = "Qingfeng Xia, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package result_mechanical
#  \ingroup FEM
#  \brief mechanical result object

from . import base_fempythonobject


class ResultMechanical(base_fempythonobject.BaseFemPythonObject):
    """
    The Fem::ResultMechanical's Proxy python type, add result specific properties
    """

    Type = "Fem::ResultMechanical"

    def __init__(self, obj):
        super().__init__(obj)

        obj.addProperty(
            "App::PropertyString",
            "ResultType",
            "Base",
            "Type of the result",
            1,  # the 1 set the property to ReadOnly
        )
        obj.setPropertyStatus("ResultType", "LockDynamic")
        obj.ResultType = str(self.Type)

        # for frequency analysis
        obj.addProperty("App::PropertyInteger", "Eigenmode", "Data", "", True)
        obj.setPropertyStatus("Eigenmode", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloat", "EigenmodeFrequency", "Data", "User Defined Results", True
        )
        obj.setPropertyStatus("EigenmodeFrequency", "LockDynamic")

        # node results
        # set read only or hide a property:
        # https://forum.freecad.org/viewtopic.php?f=18&t=13460&start=10#p108072
        # do not show up in propertyEditor of comboView
        obj.addProperty(
            "App::PropertyVectorList",
            "DisplacementVectors",
            "NodeData",
            "List of displacement vectors",
            True,
        )
        obj.setPropertyStatus("DisplacementVectors", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList",
            "Peeq",
            "NodeData",
            "List of equivalent plastic strain values",
            True,
        )
        obj.setPropertyStatus("Peeq", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList",
            "MohrCoulomb",
            "NodeData",
            "List of Mohr Coulomb stress values",
            True,
        )
        obj.setPropertyStatus("MohrCoulomb", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList",
            "ReinforcementRatio_x",
            "NodeData",
            "Reinforcement ratio x-direction",
            True,
        )
        obj.setPropertyStatus("ReinforcementRatio_x", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList",
            "ReinforcementRatio_y",
            "NodeData",
            "Reinforcement ratio y-direction",
            True,
        )
        obj.setPropertyStatus("ReinforcementRatio_y", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList",
            "ReinforcementRatio_z",
            "NodeData",
            "Reinforcement ratio z-direction",
            True,
        )
        obj.setPropertyStatus("ReinforcementRatio_z", "LockDynamic")
        # these three principal vectors are used only if there is a reinforced mat obj
        # https://forum.freecad.org/viewtopic.php?f=18&t=33106&p=416006#p416006
        obj.addProperty(
            "App::PropertyVectorList",
            "PS1Vector",
            "NodeData",
            "List of 1st Principal Stress Vectors",
            True,
        )
        obj.setPropertyStatus("PS1Vector", "LockDynamic")
        obj.addProperty(
            "App::PropertyVectorList",
            "PS2Vector",
            "NodeData",
            "List of 2nd Principal Stress Vectors",
            True,
        )
        obj.setPropertyStatus("PS2Vector", "LockDynamic")
        obj.addProperty(
            "App::PropertyVectorList",
            "PS3Vector",
            "NodeData",
            "List of 3rd Principal Stress Vectors",
            True,
        )
        obj.setPropertyStatus("PS3Vector", "LockDynamic")

        # readonly in propertyEditor of comboView
        obj.addProperty(
            "App::PropertyFloatList",
            "DisplacementLengths",
            "NodeData",
            "List of displacement lengths",
            True,
        )
        obj.setPropertyStatus("DisplacementLengths", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList",
            "vonMises",
            "NodeData",
            "List of von Mises equivalent stresses",
            True,
        )
        obj.setPropertyStatus("vonMises", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "PrincipalMax", "NodeData", "", True)
        obj.setPropertyStatus("PrincipalMax", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "PrincipalMed", "NodeData", "", True)
        obj.setPropertyStatus("PrincipalMed", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "PrincipalMin", "NodeData", "", True)
        obj.setPropertyStatus("PrincipalMin", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList",
            "MaxShear",
            "NodeData",
            "List of Maximum Shear stress values",
            True,
        )
        obj.setPropertyStatus("MaxShear", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList",
            "MassFlowRate",
            "NodeData",
            "List of mass flow rate values",
            True,
        )
        obj.setPropertyStatus("MassFlowRate", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList",
            "NetworkPressure",
            "NodeData",
            "List of network pressure values",
            True,
        )
        obj.setPropertyStatus("NetworkPressure", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList", "UserDefined", "NodeData", "User Defined Results", True
        )
        obj.setPropertyStatus("UserDefined", "LockDynamic")
        obj.addProperty(
            "App::PropertyFloatList", "Temperature", "NodeData", "Temperature field", True
        )
        obj.addProperty(
            "App::PropertyVectorList", "HeatFlux", "NodeData", "List of heat flux vectors", True
        )
        obj.setPropertyStatus("HeatFlux", "LockDynamic")

        obj.setPropertyStatus("Temperature", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStressXX", "NodeData", "", True)
        obj.setPropertyStatus("NodeStressXX", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStressYY", "NodeData", "", True)
        obj.setPropertyStatus("NodeStressYY", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStressZZ", "NodeData", "", True)
        obj.setPropertyStatus("NodeStressZZ", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStressXY", "NodeData", "", True)
        obj.setPropertyStatus("NodeStressXY", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStressXZ", "NodeData", "", True)
        obj.setPropertyStatus("NodeStressXZ", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStressYZ", "NodeData", "", True)
        obj.setPropertyStatus("NodeStressYZ", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStrainXX", "NodeData", "", True)
        obj.setPropertyStatus("NodeStrainXX", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStrainYY", "NodeData", "", True)
        obj.setPropertyStatus("NodeStrainYY", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStrainZZ", "NodeData", "", True)
        obj.setPropertyStatus("NodeStrainZZ", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStrainXY", "NodeData", "", True)
        obj.setPropertyStatus("NodeStrainXY", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStrainXZ", "NodeData", "", True)
        obj.setPropertyStatus("NodeStrainXZ", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "NodeStrainYZ", "NodeData", "", True)
        obj.setPropertyStatus("NodeStrainYZ", "LockDynamic")
        obj.addProperty("App::PropertyFloatList", "CriticalStrainRatio", "NodeData", "", True)
        obj.setPropertyStatus("CriticalStrainRatio", "LockDynamic")

        # initialize the Stats with the appropriate count of items
        # see fill_femresult_stats in femresult/resulttools.py
        zero_list = 26 * [0]
        obj.Stats = zero_list

    def onDocumentRestored(self, obj):
        # migrate old result objects, because property "StressValues"
        # was renamed to "vonMises" in commit 8b68ab7
        if hasattr(obj, "StressValues") is True:
            obj.addProperty(
                "App::PropertyFloatList",
                "vonMises",
                "NodeData",
                "List of von Mises equivalent stresses",
                True,
            )
            obj.setPropertyStatus("vonMises", "LockDynamic")
            obj.vonMises = obj.StressValues
            obj.setPropertyStatus("StressValues", "-LockDynamic")
            obj.removeProperty("StressValues")

        # migrate old result objects, because property "Stats"
        # consisting of min, avg, max values was reduced to min, max in commit c2a57b3e
        if len(obj.Stats) == 39:
            temp = obj.Stats
            for i in range(12, -1, -1):
                del temp[3 * i + 1]
            obj.Stats = temp
