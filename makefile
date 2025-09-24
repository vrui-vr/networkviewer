########################################################################
# Makefile for Network Viewer.
# Copyright (c) 2019-2025 Oliver Kreylos
#
# This file is part of the WhyTools Build Environment.
# 
# The WhyTools Build Environment is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The WhyTools Build Environment is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the WhyTools Build Environment; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

# Directory containing the Vrui build system. The directory below
# matches the default Vrui installation; if Vrui's installation
# directory was changed during Vrui's installation, the directory below
# must be adapted.
VRUI_MAKEDIR = /usr/local/share/Vrui-13.1/make

# Base installation directory for the Network Viewer. If this is set to
# the default of $(PROJECT_ROOT), the Network Viewer does not have to be
# installed to be run. Created libraries and plug-ins, executables,
# configuration files, and resources will be installed in the lib, bin,
# etc, and share directories under the given base directory,
# respectively.
# Important note: Do not use ~ as an abbreviation for the user's home
# directory here; use $(HOME) instead.
INSTALLDIR = $(PROJECT_ROOT)

########################################################################
# Everything below here should not have to be changed
########################################################################

PROJECT_NAME = NetworkViewer
PROJECT_DISPLAYNAME = Network Viewer

# Version number for installation subdirectories. This is used to keep
# subsequent release versions of the Network Viewer from clobbering each
# other. The value should be identical to the major.minor version number
# found in VERSION in the root package directory.
PROJECT_MAJOR = 3
PROJECT_MINOR = 1

# Set up resource directories: */
CONFIGDIR = etc/NetworkViewer-$(VERSION)
RESOURCEDIR = share/NetworkViewer-$(VERSION)

# Include definitions for the system environment and system-provided
# packages
include $(VRUI_MAKEDIR)/SystemDefinitions
include $(VRUI_MAKEDIR)/Packages.System
include $(VRUI_MAKEDIR)/Configuration.Vrui
include $(VRUI_MAKEDIR)/Packages.Vrui
-include $(VRUI_MAKEDIR)/Configuration.Collaboration
-include $(VRUI_MAKEDIR)/Packages.Collaboration

# Determine if the local OpenGL supports the GL_ARB_geometry_shader4
# extension to support impostor spheres:
SYSTEM_HAVE_GEOMETRYSHADER = 0
ifneq ($(strip $(shell glxinfo -s | grep GL_ARB_geometry_shader4)),)
  SYSTEM_HAVE_GEOMETRYSHADER = 1
endif

########################################################################
# Specify additional compiler and linker flags
########################################################################

CFLAGS += -Wall -pedantic

########################################################################
# List common packages used by all components of this project
# (Supported packages can be found in $(VRUI_MAKEDIR)/Packages.*)
########################################################################

PACKAGES = MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYIO MYREALTIME MYMISC GL

########################################################################
# Specify all final targets
########################################################################

CONFIGFILES = 
EXECUTABLES = 
COLLABORATIONPLUGINS = 

CONFIGFILES += Config.h

EXECUTABLES += $(EXEDIR)/ParticleTest \
               $(EXEDIR)/NetworkViewer

ifdef COLLABORATION_VERSION
  # Buld the Collaborative Network Viewer
  EXECUTABLES += $(EXEDIR)/CollaborativeNetworkViewer
  
  # Build the Network Viewer server-side collaboration plug-in
  NETWORKVIEWER_NAME = NetworkViewer
  NETWORKVIEWER_VERSION = 4
  COLLABORATIONPLUGINS += $(call COLLABORATIONPLUGIN_SERVER_TARGET,NETWORKVIEWER)
endif

ALL = $(EXECUTABLES) $(COLLABORATIONPLUGINS)

.PHONY: all
all: $(ALL)

########################################################################
# Pseudo-target to print configuration options and configure the package
########################################################################

.PHONY: config config-invalidate
config: config-invalidate $(DEPDIR)/config

config-invalidate:
	@mkdir -p $(DEPDIR)
	@touch $(DEPDIR)/Configure-Begin

$(DEPDIR)/Configure-Begin:
	@mkdir -p $(DEPDIR)
	@echo "---- $(PROJECT_FULLDISPLAYNAME) configuration options: ----"
	@touch $(DEPDIR)/Configure-Begin

$(DEPDIR)/Configure-Package: $(DEPDIR)/Configure-Begin
ifneq ($(SYSTEM_HAVE_GEOMETRYSHADER),0)
	@echo "Impostor sphere rendering enabled"
else
	@echo "Impostor sphere rendering disabled"
endif
ifdef COLLABORATION_VERSION
	@echo "Collaborative visualization enabled"
else
	@echo "Collaborative visualization disabled"
endif
	@touch $(DEPDIR)/Configure-Package

$(DEPDIR)/Configure-Install: $(DEPDIR)/Configure-Package
	@echo "---- $(PROJECT_FULLDISPLAYNAME) installation configuration ----"
	@echo "Installation directory: $(INSTALLDIR)"
	@echo "Executable directory: $(EXECUTABLEINSTALLDIR)"
	@echo "Configuration directory: $(ETCINSTALLDIR)"
ifdef COLLABORATION_VERSION
	@echo "Collaboration plug-in directory: $(COLLABORATIONPLUGINS_LIBDIR)"
endif
	@touch $(DEPDIR)/Configure-Install

$(DEPDIR)/Configure-End: $(DEPDIR)/Configure-Install
	@echo "---- End of $(PROJECT_FULLDISPLAYNAME) configuration options ----"
	@touch $(DEPDIR)/Configure-End

Config.h: | $(DEPDIR)/Configure-End
	@echo "Creating Config.h configuration file"
	@cp Config.h.template Config.h.temp
	@$(call CONFIG_SETVAR,Config.h.temp,CONFIG_USE_IMPOSTORSPHERES,$(SYSTEM_HAVE_GEOMETRYSHADER))
	@if ! diff -qN Config.h.temp Config.h > /dev/null ; then cp Config.h.temp Config.h ; fi
	@rm Config.h.temp

$(DEPDIR)/config: $(DEPDIR)/Configure-End $(CONFIGFILES)
	@touch $(DEPDIR)/config

########################################################################
# Specify other actions to be performed on a `make clean'
########################################################################

.PHONY: extraclean
extraclean:

.PHONY: extrasqueakyclean
extrasqueakyclean:
	-rm $(CONFIGFILES)

# Include basic makefile
include $(VRUI_MAKEDIR)/BasicMakefile

########################################################################
# Specify build rules for executables
########################################################################

PARTICLETEST_SOURCES = ParticleOctree.cpp \
                       ParticleSystem.cpp \
                       ParticleMesh.cpp \
                       Body.cpp \
                       Whip.cpp \
                       Figure.cpp \
                       ParticleGrabber.cpp \
                       ParticleTest.cpp

$(PARTICLETEST_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

$(EXEDIR)/ParticleTest: PACKAGES += MYVRUI MYGLMOTIF MYGLGEOMETRY
$(EXEDIR)/ParticleTest: $(PARTICLETEST_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: ParticleTest
ParticleTest: $(EXEDIR)/ParticleTest

#
# Old non-collaborative Network Viewer
#

NETWORK_SOURCES = ParticleOctree.cpp \
                  ParticleSystem.cpp \
                  JsonFile.cpp \
                  Node.cpp \
                  Network.cpp \
                  SimulationParameters.cpp \
                  NetworkSimulator.cpp

NETWORKVIEWER_SOURCES = $(NETWORK_SOURCES) \
                        NetworkViewerTool.cpp \
                        SelectAndDragTool.cpp \
                        AddSelectTool.cpp \
                        SubtractSelectTool.cpp \
                        ShowPropertiesTool.cpp \
                        NetworkViewer.cpp

$(NETWORKVIEWER_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

$(EXEDIR)/NetworkViewer: PACKAGES += MYVRUI MYGLMOTIF MYGLGEOMETRY MYTHREADS
$(EXEDIR)/NetworkViewer: $(NETWORKVIEWER_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: NetworkViewer
NetworkViewer: $(EXEDIR)/NetworkViewer

#
# Collaborative Network Viewer
#

COLLABORATIVENETWORKVIEWER_SOURCES = $(NETWORK_SOURCES) \
                                     RenderingParameters.cpp \
                                     NetworkViewerProtocol.cpp \
                                     NetworkViewerClient.cpp \
                                     NetworkViewerClientTool.cpp \
                                     NetworkViewerClientSelectTool.cpp \
                                     NetworkViewerClientDeselectTool.cpp \
                                     NetworkViewerClientToggleSelectTool.cpp \
                                     CreateNodeLabel.cpp \
                                     NetworkViewerClientShowLabelTool.cpp \
                                     NetworkViewerClientDragTool.cpp \
                                     CollaborativeNetworkViewer.cpp

$(COLLABORATIVENETWORKVIEWER_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

$(EXEDIR)/CollaborativeNetworkViewer: PACKAGES += MYCOLLABORATION2CLIENT MYVRUI MYSCENEGRAPH MYGLMOTIF MYGLGEOMETRY MYTHREADS
$(EXEDIR)/CollaborativeNetworkViewer: $(COLLABORATIVENETWORKVIEWER_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: CollaborativeNetworkViewer
CollaborativeNetworkViewer: $(EXEDIR)/CollaborativeNetworkViewer

#
# Collaborative Network Viewer server plug-in
#

NETWORKVIEWERSERVER_SOURCES = $(NETWORK_SOURCES) \
                              RenderingParameters.cpp \
                              NetworkViewerProtocol.cpp \
                              NetworkViewerServer.cpp

$(call PLUGINOBJNAMES,$(NETWORKVIEWERSERVER_SOURCES)): | $(DEPDIR)/config

$(call COLLABORATIONPLUGIN_SERVER_TARGET,NETWORKVIEWER): PACKAGES += MYCOLLABORATION2SERVER MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYIO MYTHREADS MYREALTIME MYMISC GL
$(call COLLABORATIONPLUGIN_SERVER_TARGET,NETWORKVIEWER): $(call PLUGINOBJNAMES,$(NETWORKVIEWERSERVER_SOURCES))
.PHONY: NetworkViewerServer
NetworkViewerServer: $(call PLUGIN_SERVER,NETWORKVIEWER)

########################################################################
# Specify installation rules for header files, libraries, executables,
# configuration files, and shared files.
########################################################################

install: $(ALL)
	@echo Installing $(PROJECT_DISPLAYNAME) in $(INSTALLDIR)...
	@install -d $(INSTALLDIR)
	@install -d $(EXECUTABLEINSTALLDIR)
	@install $(EXECUTABLES) $(EXECUTABLEINSTALLDIR)

installplugins: $(COLLABORATIONPLUGINS)
ifdef COLLABORATION_VERSION
	@echo Installing $(PROJECT_DISPLAYNAME)\'s collaboration plug-ins in $(COLLABORATIONPLUGINS_LIBDIR)...
	@install $(COLLABORATIONPLUGINS) $(COLLABORATIONPLUGINS_LIBDIR)
else
	@echo Collaboration features disabled
endif
