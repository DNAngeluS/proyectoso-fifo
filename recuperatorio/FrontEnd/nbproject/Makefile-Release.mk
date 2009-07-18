#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=g77

# Macros
PLATFORM=GNU-Solaris-x86

# Include project Makefile
include FrontEnd-Makefile.mk

# Object Directory
OBJECTDIR=build/Release/${PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/log.o \
	${OBJECTDIR}/frontend.o \
	${OBJECTDIR}/config.o \
	${OBJECTDIR}/irc.o \
	${OBJECTDIR}/http.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Release.mk dist/Release/${PLATFORM}/frontend

dist/Release/${PLATFORM}/frontend: ${OBJECTFILES}
	${MKDIR} -p dist/Release/${PLATFORM}
	${LINK.c} -o dist/Release/${PLATFORM}/frontend ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/log.o: log.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/log.o log.c

${OBJECTDIR}/frontend.o: frontend.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/frontend.o frontend.c

${OBJECTDIR}/config.o: config.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/config.o config.c

${OBJECTDIR}/irc.o: irc.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/irc.o irc.c

${OBJECTDIR}/http.o: http.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/http.o http.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Release
	${RM} dist/Release/${PLATFORM}/frontend

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
