#  IBM_PROLOG_BEGIN_TAG
#  This is an automatically generated prolog.
#
#  $Source: src/usr/targeting/makefile $
#
#  IBM CONFIDENTIAL
#
#  COPYRIGHT International Business Machines Corp. 2011
#
#  p1
#
#  Object Code Only (OCO) source materials
#  Licensed Internal Code Source Materials
#  IBM HostBoot Licensed Internal Code
#
#  The source code for this program is not published or other-
#  wise divested of its trade secrets, irrespective of what has
#  been deposited with the U.S. Copyright Office.
#
#  Origin: 30
#
#  IBM_PROLOG_END

################################################################################
#
#  @file targeting/common/common.mk
#
#  @brief Common targeting makefile to be included in 
#      targeting/makefile 
#
################################################################################

COMMON_TARGETING_SUBDIRS = predicates iterators

PREDICATES_OBJS =           \
    predicatebase.o         \
    predicatepostfixexpr.o  \
    predicatectm.o          \
    predicateisfunctional.o  
    
ITERATORS_OBJS =        \
    targetiterator.o    \
    rangefilter.o

TARGET_OBJS =       \
    target.o        \
    targetservice.o \
    entitypath.o

OTHER_OBJS = \
     util.o
     
# Common     
COMMON_TARGETING_OBJS = \
    ${TARGET_OBJS}      \
    ${PREDICATES_OBJS}  \
    ${ITERATORS_OBJS}   \
    ${OTHER_OBJS}
