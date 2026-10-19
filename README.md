# GEMSTAR UE_esmini_plugin

[![DLR-TS](https://img.shields.io/badge/developed%20by-DLR%20TS-orange.svg?style=flat&colorA=E1523D&colorB=007D8A)](https://www.dlr.de/en/ts/about-us/the-institute-of-transportation-systems)

The esmini plugins enables GEMSTAR to use [esmini](https://github.com/esmini/esmini) `v3.5.0` as a scenario runner. It replay OpenSCENARIO files and distribute the states of vehicles and pedestrians to other plugins.

## Input

- OpenScenario file
- Weather xml file, a xml file with a OpenSCENARIO Environmental parameters. 

## Output

- Vehicle Data Update
- VRU Data Update
- Weather configuration

# Getting started

First clone the GEMSTAR Interfaces plugin to Plugins directory of the Unreal project.

Add the plugin to your Unreal project:

```sh
mkdir Plugins/GameFeatures
cd Plugins/GameFeatures

git clone https://github.com/DLR-TS/ue_esmini_feature.git
```
or:
```sh
git submodule add https://github.com/DLR-TS/ue_esmini_feature.git
```

During compile time a sample section from Resources/GemstarFeature.ini is copied to the Simulation/DefaultGemstar.ini file. This happens every time the plugin is compiled, but existing sections do not will be overwritten. So your changed will remain. Also the Resources/OpenSCENARIO folder will be copied to Simulations to give you some examples.

# Configuration

You can configure the OpenScenario file to be played by setting the Path variable of EsminiManager actor or use the DefaultGemstar.ini file [ue_esmini_feature] section to set the Path variable. The path can be absolute or relative to the Simulation/OpenSCENARIO folder.

### Weather information

This plugin tries to get weather information from a OpenSCENARIO Environment tag. When the scenario is loading  by the EsminiManager it looks for a this tag in three different places in the follow order:
- A file with the name pattern weather_<scenario_name>.xml in the same folder as the scenario file. E.g. cut-in.xosc would look for weather_cut-in.xml.
- Any file that follow the name pattern weather_*.xml in the same folder as the scenario file. The use case is on weather information for different scenarios.
- Inside of the scenario itself under the path Storyboard.Init.Actions.GlobalAction.EnvironmentAction.Environment.

When there is a weather information, EsminiManager will send a FWeatherData message to every plugin that offers GEMSTARInterfaceType::WEATHERDATA as input.

## Third-party dependencies

Linux artefacts are built through Docker:

```sh
./Scripts/build-dependencies.sh
```

Windows artefacts are built natively with Git, CMake, Visual Studio 2022:

```powershell
.\Scripts\build-dependencies.ps1
```

# Test

![Esmini parameters in Unreal](Documentation/esmini_configuration.png)

A test map is provided in the Content directory.
It shows the transforms of the vehicles changing during the simulation.

---------------
**Contact:** opensource-ts@dlr.de

This project uses [esmini](https://github.com/esmini/esmini), which is licensed under the
Mozilla Public License 2.0.