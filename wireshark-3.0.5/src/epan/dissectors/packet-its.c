/* Do not modify this file. Changes will be overwritten.                      */
/* Generated automatically by the ASN.1 to Wireshark dissector compiler       */
/* packet-its.c                                                               */
/* asn2wrs.py -o its -c ./its.cnf -s ./packet-its-template -D . -O ../.. ITS-Container.asn ISO_TS_14816.asn ISO_TS_24534-3.asn ISO_TS_17419.asn ISO_TS_14906_Application.asn ISO_TS_19321.asn ISO_TS_19091.asn ETSI_TS_103301.asn CAM.asn DENM.asn TIS_TPG_Transactions_Descriptions.asn EVCSN-PDU-Descriptions.asn EV-RSR-PDU-Descriptions.asn */

/* Input file: packet-its-template.c */

#line 1 "./asn1/its/packet-its-template.c"
/* packet-its-template.c
 *
 * Intelligent Transport Systems Applications dissectors
 * Coyright 2018, C. Guerber <cguerber@yahoo.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Implemented:
 * CA (CAM)                           ETSI EN 302 637-2
 * DEN (DENM)                         ETSI EN 302 637-3
 * RLT (MAPEM)                        ETSI TS 103 301
 * TLM (SPATEM)                       ETSI TS 103 301
 * IVI (IVIM)                         ETSI TS 103 301
 * TLC (SREM)                         ETSI TS 103 301
 * TLC (SSEM)                         ETSI TS 103 301
 * EVCSN POI (EVCSN POI message)      ETSI TS 101 556-1
 * TPG (TRM, TCM, VDRM, VDPM, EOFM)   ETSI TS 101 556-2
 * Charging (EV-RSR, SRM, SCM)        ETSI TS 101 556-3
 *
 * Not supported:
 * SA (SAEM)                          ETSI TS 102 890-1
 * GPC (RTCMEM)                       ETSI TS 103 301
 * CTL (CTLM)                         ETSI TS 102 941
 * CRL (CRLM)                         ETSI TS 102 941
 * Certificate request                ETSI TS 102 941
 */
#include "config.h"

#include <epan/packet.h>
#include <epan/expert.h>
#include <epan/decode_as.h>
#include <epan/proto_data.h>
#include <epan/exceptions.h>
#include <epan/conversation.h>
#include <epan/tap.h>
#include <wsutil/utf8_entities.h>
#include "packet-ber.h"
#include "packet-per.h"

#include "packet-its.h"
#include "packet-ieee1609dot2.h"

/*
 * Well Known Ports definitions as per:
 *
 * ETSI TS 103 248 v1.2.1 (2018-08)
 * Intelligent Transport Systems (ITS);
 * GeoNetworking;
 * Port Numbers for the Basic Transport Protocol (BTP)
 *
 * BTP port   Facilities service      Related standard
 * number     or Application
 * values
 * 2001       CA (CAM)                ETSI EN 302 637-2
 * 2002       DEN (DENM)              ETSI EN 302 637-3
 * 2003       RLT (MAPEM)             ETSI TS 103 301
 * 2004       TLM (SPATEM)            ETSI TS 103 301
 * 2005       SA (SAEM)               ETSI TS 102 890-1
 * 2006       IVI (IVIM)              ETSI TS 103 301
 * 2007       TLC (SREM)              ETSI TS 103 301
 * 2008       TLC (SSEM)              ETSI TS 103 301
 * 2009       Allocated               Allocated for "Intelligent Transport
 *                                    System (ITS); Vehicular Communications;
 *                                    Basic Set of Applications; Specification
 *                                    of the Collective Perception Service"
 * 2010       EVCSN POI (EVCSN POI    ETSI TS 101 556-1
 *            message)
 * 2011       TPG (TRM, TCM, VDRM,    ETSI TS 101 556-2
 *            VDPM, EOFM)
 * 2012       Charging (EV-RSR,       ETSI TS 101 556-3
 *            SRM, SCM)
 * 2013       GPC (RTCMEM)            ETSI TS 103 301
 * 2014       CTL (CTLM)              ETSI TS 102 941
 * 2015       CRL (CRLM)              ETSI TS 102 941
 * 2016       Certificate request     ETSI TS 102 941
 */

// Applications Well Known Ports
#define ITS_WKP_CA         2001
#define ITS_WKP_DEN        2002
#define ITS_WKP_RLT        2003
#define ITS_WKP_TLM        2004
#define ITS_WKP_SA         2005
#define ITS_WKP_IVI        2006
#define ITS_WKP_TLC_SREM   2007
#define ITS_WKP_TLC_SSEM   2008
#define ITS_WKP_CPS        2009
#define ITS_WKP_EVCSN      2010
#define ITS_WKP_TPG        2011
#define ITS_WKP_CHARGING   2012
#define ITS_WKP_GPC        2013
#define ITS_WKP_CTL        2014
#define ITS_WKP_CRL        2015
#define ITS_WKP_CERTIF_REQ 2016

/*
 * Prototypes
 */
void proto_reg_handoff_its(void);
void proto_register_its(void);

// TAP
static int its_tap = -1;

// Protocols
static int proto_its = -1;
static int proto_its_denm = -1;
static int proto_its_cam = -1;
static int proto_its_evcsn = -1;
static int proto_its_evrsr = -1;
static int proto_its_ivim = -1;
static int proto_its_tistpg = -1;
static int proto_its_ssem = -1;
static int proto_its_srem = -1;
static int proto_its_mapem = -1;
static int proto_its_spatem = -1;
static int proto_addgrpc = -1;

/*
 * DENM SSP
 */
static int hf_denmssp_version = -1;
static int hf_denmssp_flags = -1;
static int hf_denmssp_trafficCondition = -1;
static int hf_denmssp_accident = -1;
static int hf_denmssp_roadworks = -1;
static int hf_denmssp_adverseWeatherConditionAdhesion = -1;
static int hf_denmssp_hazardousLocationSurfaceCondition = -1;
static int hf_denmssp_hazardousLocationObstacleOnTheRoad = -1;
static int hf_denmssp_hazardousLocationAnimalOnTheRoad = -1;
static int hf_denmssp_humanPresenceOnTheRoad = -1;
static int hf_denmssp_wrongWayDriving = -1;
static int hf_denmssp_rescueAndRecoveryWorkInProgress = -1;
static int hf_denmssp_ExtremeWeatherCondition = -1;
static int hf_denmssp_adverseWeatherConditionVisibility = -1;
static int hf_denmssp_adverseWeatherConditionPrecipitation = -1;
static int hf_denmssp_slowVehicle = -1;
static int hf_denmssp_dangerousEndOfQueue = -1;
static int hf_denmssp_vehicleBreakdown = -1;
static int hf_denmssp_postCrash = -1;
static int hf_denmssp_humanProblem = -1;
static int hf_denmssp_stationaryVehicle = -1;
static int hf_denmssp_emergencyVehicleApproaching = -1;
static int hf_denmssp_hazardousLocationDangerousCurve = -1;
static int hf_denmssp_collisionRisk = -1;
static int hf_denmssp_signalViolation = -1;
static int hf_denmssp_dangerousSituation = -1;

/*
 * CAM SSP
 */
static int hf_camssp_version = -1;
static int hf_camssp_flags = -1;
static int hf_camssp_cenDsrcTollingZone = -1;
static int hf_camssp_publicTransport = -1;
static int hf_camssp_specialTransport = -1;
static int hf_camssp_dangerousGoods = -1;
static int hf_camssp_roadwork = -1;
static int hf_camssp_rescue = -1;
static int hf_camssp_emergency = -1;
static int hf_camssp_safetyCar = -1;
static int hf_camssp_closedLanes = -1;
static int hf_camssp_requestForRightOfWay = -1;
static int hf_camssp_requestForFreeCrossingAtATrafficLight = -1;
static int hf_camssp_noPassing = -1;
static int hf_camssp_noPassingForTrucks = -1;
static int hf_camssp_speedLimit = -1;
static int hf_camssp_reserved = -1;

static gint ett_denmssp_flags = -1;
static gint ett_camssp_flags = -1;

// Subdissectors
static dissector_table_t its_version_subdissector_table;
static dissector_table_t its_msgid_subdissector_table;
static dissector_table_t regionid_subdissector_table;

typedef struct its_private_data {
    enum regext_type_enum type;
    guint32 region_id;
    guint32 cause_code;
} its_private_data_t;

// Specidic dissector for content of open type for regional extensions
static int dissect_regextval_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    its_private_data_t *re = (its_private_data_t*)data;
    // XXX What to do when region_id = noRegion? Test length is zero?
    if (!dissector_try_uint_new(regionid_subdissector_table, ((guint32) re->region_id<<16) + (guint32) re->type, tvb, pinfo, tree, FALSE, NULL))
        call_data_dissector(tvb, pinfo, tree);
    return tvb_captured_length(tvb);
}

static int dissect_denmssp_pdu(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void *data _U_)
{
    static const int *denmssp_flags[] = {
        &hf_denmssp_trafficCondition,
        &hf_denmssp_accident,
        &hf_denmssp_roadworks,
        &hf_denmssp_adverseWeatherConditionAdhesion,
        &hf_denmssp_hazardousLocationSurfaceCondition,
        &hf_denmssp_hazardousLocationObstacleOnTheRoad,
        &hf_denmssp_hazardousLocationAnimalOnTheRoad,
        &hf_denmssp_humanPresenceOnTheRoad,
        &hf_denmssp_wrongWayDriving,
        &hf_denmssp_rescueAndRecoveryWorkInProgress,
        &hf_denmssp_ExtremeWeatherCondition,
        &hf_denmssp_adverseWeatherConditionVisibility,
        &hf_denmssp_adverseWeatherConditionPrecipitation,
        &hf_denmssp_slowVehicle,
        &hf_denmssp_dangerousEndOfQueue,
        &hf_denmssp_vehicleBreakdown,
        &hf_denmssp_postCrash,
        &hf_denmssp_humanProblem,
        &hf_denmssp_stationaryVehicle,
        &hf_denmssp_emergencyVehicleApproaching,
        &hf_denmssp_hazardousLocationDangerousCurve,
        &hf_denmssp_collisionRisk,
        &hf_denmssp_signalViolation,
        &hf_denmssp_dangerousSituation,
        NULL
    };

    guint32 version;

    proto_tree_add_item_ret_uint(tree, hf_denmssp_version, tvb, 0, 1, ENC_BIG_ENDIAN, &version);
    if (version == 1) {
        proto_tree_add_bitmask(tree, tvb, 1, hf_denmssp_flags, ett_denmssp_flags, denmssp_flags, ENC_BIG_ENDIAN);
    }
    return tvb_reported_length(tvb);
}

static int dissect_camssp_pdu(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void *data _U_)
{
    static const int *camssp_flags[] = {
        &hf_camssp_cenDsrcTollingZone,
        &hf_camssp_publicTransport,
        &hf_camssp_specialTransport,
        &hf_camssp_dangerousGoods,
        &hf_camssp_roadwork,
        &hf_camssp_rescue,
        &hf_camssp_emergency,
        &hf_camssp_safetyCar,
        &hf_camssp_closedLanes,
        &hf_camssp_requestForRightOfWay,
        &hf_camssp_requestForFreeCrossingAtATrafficLight,
        &hf_camssp_noPassing,
        &hf_camssp_noPassingForTrucks,
        &hf_camssp_speedLimit,
        &hf_camssp_reserved,
        NULL
    };

    guint32 version;

    proto_tree_add_item_ret_uint(tree, hf_camssp_version, tvb, 0, 1, ENC_BIG_ENDIAN, &version);
    if (version == 1) {
        proto_tree_add_bitmask(tree, tvb, 1, hf_camssp_flags, ett_camssp_flags, camssp_flags, ENC_BIG_ENDIAN);
    }
    return tvb_reported_length(tvb);
}

// Generated by asn2wrs

/*--- Included file: packet-its-hf.c ---*/
#line 1 "./asn1/its/packet-its-hf.c"

/* --- Module ITS-Container --- --- ---                                       */

static int hf_its_its_ItsPduHeader_PDU = -1;      /* ItsPduHeader */
static int hf_its_protocolVersion = -1;           /* T_protocolVersion */
static int hf_its_messageID = -1;                 /* T_messageID */
static int hf_its_stationID = -1;                 /* StationID */
static int hf_its_latitude = -1;                  /* Latitude */
static int hf_its_longitude = -1;                 /* Longitude */
static int hf_its_positionConfidenceEllipse = -1;  /* PosConfidenceEllipse */
static int hf_its_altitude = -1;                  /* Altitude */
static int hf_its_deltaLatitude = -1;             /* DeltaLatitude */
static int hf_its_deltaLongitude = -1;            /* DeltaLongitude */
static int hf_its_deltaAltitude = -1;             /* DeltaAltitude */
static int hf_its_altitudeValue = -1;             /* AltitudeValue */
static int hf_its_altitudeConfidence = -1;        /* AltitudeConfidence */
static int hf_its_semiMajorConfidence = -1;       /* SemiAxisLength */
static int hf_its_semiMinorConfidence = -1;       /* SemiAxisLength */
static int hf_its_semiMajorOrientation = -1;      /* HeadingValue */
static int hf_its_pathPosition = -1;              /* DeltaReferencePosition */
static int hf_its_pathDeltaTime = -1;             /* PathDeltaTime */
static int hf_its_ptActivationType = -1;          /* PtActivationType */
static int hf_its_ptActivationData = -1;          /* PtActivationData */
static int hf_its_causeCode = -1;                 /* CauseCodeType */
static int hf_its_subCauseCode = -1;              /* SubCauseCodeType */
static int hf_its_curvatureValue = -1;            /* CurvatureValue */
static int hf_its_curvatureConfidence = -1;       /* CurvatureConfidence */
static int hf_its_headingValue = -1;              /* HeadingValue */
static int hf_its_headingConfidence = -1;         /* HeadingConfidence */
static int hf_its_innerhardShoulderStatus = -1;   /* HardShoulderStatus */
static int hf_its_outerhardShoulderStatus = -1;   /* HardShoulderStatus */
static int hf_its_drivingLaneStatus = -1;         /* DrivingLaneStatus */
static int hf_its_speedValue = -1;                /* SpeedValue */
static int hf_its_speedConfidence = -1;           /* SpeedConfidence */
static int hf_its_longitudinalAccelerationValue = -1;  /* LongitudinalAccelerationValue */
static int hf_its_longitudinalAccelerationConfidence = -1;  /* AccelerationConfidence */
static int hf_its_lateralAccelerationValue = -1;  /* LateralAccelerationValue */
static int hf_its_lateralAccelerationConfidence = -1;  /* AccelerationConfidence */
static int hf_its_verticalAccelerationValue = -1;  /* VerticalAccelerationValue */
static int hf_its_verticalAccelerationConfidence = -1;  /* AccelerationConfidence */
static int hf_its_dangerousGoodsType = -1;        /* DangerousGoodsBasic */
static int hf_its_unNumber = -1;                  /* INTEGER_0_9999 */
static int hf_its_elevatedTemperature = -1;       /* BOOLEAN */
static int hf_its_tunnelsRestricted = -1;         /* BOOLEAN */
static int hf_its_limitedQuantity = -1;           /* BOOLEAN */
static int hf_its_emergencyActionCode = -1;       /* IA5String_SIZE_1_24 */
static int hf_its_phoneNumber = -1;               /* PhoneNumber */
static int hf_its_companyName = -1;               /* UTF8String_SIZE_1_24 */
static int hf_its_wMInumber = -1;                 /* WMInumber */
static int hf_its_vDS = -1;                       /* VDS */
static int hf_its_vehicleLengthValue = -1;        /* VehicleLengthValue */
static int hf_its_vehicleLengthConfidenceIndication = -1;  /* VehicleLengthConfidenceIndication */
static int hf_its_PathHistory_item = -1;          /* PathPoint */
static int hf_its_steeringWheelAngleValue = -1;   /* SteeringWheelAngleValue */
static int hf_its_steeringWheelAngleConfidence = -1;  /* SteeringWheelAngleConfidence */
static int hf_its_yawRateValue = -1;              /* YawRateValue */
static int hf_its_yawRateConfidence = -1;         /* YawRateConfidence */
static int hf_its_originatingStationID = -1;      /* StationID */
static int hf_its_sequenceNumber = -1;            /* SequenceNumber */
static int hf_its_ItineraryPath_item = -1;        /* ReferencePosition */
static int hf_its_protectedZoneType = -1;         /* ProtectedZoneType */
static int hf_its_expiryTime = -1;                /* TimestampIts */
static int hf_its_protectedZoneLatitude = -1;     /* Latitude */
static int hf_its_protectedZoneLongitude = -1;    /* Longitude */
static int hf_its_protectedZoneRadius = -1;       /* ProtectedZoneRadius */
static int hf_its_protectedZoneID = -1;           /* ProtectedZoneID */
static int hf_its_Traces_item = -1;               /* PathHistory */
static int hf_its_PositionOfPillars_item = -1;    /* PosPillar */
static int hf_its_RestrictedTypes_item = -1;      /* StationType */
static int hf_its_EventHistory_item = -1;         /* EventPoint */
static int hf_its_eventPosition = -1;             /* DeltaReferencePosition */
static int hf_its_eventDeltaTime = -1;            /* PathDeltaTime */
static int hf_its_informationQuality = -1;        /* InformationQuality */
static int hf_its_ProtectedCommunicationZonesRSU_item = -1;  /* ProtectedCommunicationZone */
static int hf_its_cenDsrcTollingZoneID = -1;      /* CenDsrcTollingZoneID */
static int hf_its_DigitalMap_item = -1;           /* ReferencePosition */
/* named bits */
static int hf_its_AccelerationControl_brakePedalEngaged = -1;
static int hf_its_AccelerationControl_gasPedalEngaged = -1;
static int hf_its_AccelerationControl_emergencyBrakeEngaged = -1;
static int hf_its_AccelerationControl_collisionWarningEngaged = -1;
static int hf_its_AccelerationControl_accEngaged = -1;
static int hf_its_AccelerationControl_cruiseControlEngaged = -1;
static int hf_its_AccelerationControl_speedLimiterEngaged = -1;
static int hf_its_ExteriorLights_lowBeamHeadlightsOn = -1;
static int hf_its_ExteriorLights_highBeamHeadlightsOn = -1;
static int hf_its_ExteriorLights_leftTurnSignalOn = -1;
static int hf_its_ExteriorLights_rightTurnSignalOn = -1;
static int hf_its_ExteriorLights_daytimeRunningLightsOn = -1;
static int hf_its_ExteriorLights_reverseLightOn = -1;
static int hf_its_ExteriorLights_fogLightOn = -1;
static int hf_its_ExteriorLights_parkingLightsOn = -1;
static int hf_its_SpecialTransportType_heavyLoad = -1;
static int hf_its_SpecialTransportType_excessWidth = -1;
static int hf_its_SpecialTransportType_excessLength = -1;
static int hf_its_SpecialTransportType_excessHeight = -1;
static int hf_its_LightBarSirenInUse_lightBarActivated = -1;
static int hf_its_LightBarSirenInUse_sirenActivated = -1;
static int hf_its_PositionOfOccupants_row1LeftOccupied = -1;
static int hf_its_PositionOfOccupants_row1RightOccupied = -1;
static int hf_its_PositionOfOccupants_row1MidOccupied = -1;
static int hf_its_PositionOfOccupants_row1NotDetectable = -1;
static int hf_its_PositionOfOccupants_row1NotPresent = -1;
static int hf_its_PositionOfOccupants_row2LeftOccupied = -1;
static int hf_its_PositionOfOccupants_row2RightOccupied = -1;
static int hf_its_PositionOfOccupants_row2MidOccupied = -1;
static int hf_its_PositionOfOccupants_row2NotDetectable = -1;
static int hf_its_PositionOfOccupants_row2NotPresent = -1;
static int hf_its_PositionOfOccupants_row3LeftOccupied = -1;
static int hf_its_PositionOfOccupants_row3RightOccupied = -1;
static int hf_its_PositionOfOccupants_row3MidOccupied = -1;
static int hf_its_PositionOfOccupants_row3NotDetectable = -1;
static int hf_its_PositionOfOccupants_row3NotPresent = -1;
static int hf_its_PositionOfOccupants_row4LeftOccupied = -1;
static int hf_its_PositionOfOccupants_row4RightOccupied = -1;
static int hf_its_PositionOfOccupants_row4MidOccupied = -1;
static int hf_its_PositionOfOccupants_row4NotDetectable = -1;
static int hf_its_PositionOfOccupants_row4NotPresent = -1;
static int hf_its_EnergyStorageType_hydrogenStorage = -1;
static int hf_its_EnergyStorageType_electricEnergyStorage = -1;
static int hf_its_EnergyStorageType_liquidPropaneGas = -1;
static int hf_its_EnergyStorageType_compressedNaturalGas = -1;
static int hf_its_EnergyStorageType_diesel = -1;
static int hf_its_EnergyStorageType_gasoline = -1;
static int hf_its_EnergyStorageType_ammonia = -1;
static int hf_its_EmergencyPriority_requestForRightOfWay = -1;
static int hf_its_EmergencyPriority_requestForFreeCrossingAtATrafficLight = -1;

/* --- Module ElectronicRegistrationIdentificationVehicleDataModule --- --- --- */

static int hf_erivdm_euVehicleCategoryL = -1;     /* EuVehicleCategoryL */
static int hf_erivdm_euVehicleCategoryM = -1;     /* EuVehicleCategoryM */
static int hf_erivdm_euVehicleCategoryN = -1;     /* EuVehicleCategoryN */
static int hf_erivdm_euVehicleCategoryO = -1;     /* EuVehicleCategoryO */
static int hf_erivdm_euVehilcleCategoryT = -1;    /* NULL */
static int hf_erivdm_euVehilcleCategoryG = -1;    /* NULL */

/* --- Module CITSapplMgmtIDs --- --- ---                                     */

static int hf_csmid_vlnContent = -1;              /* INTEGER_0_127 */
static int hf_csmid_vlnExtension = -1;            /* Ext1 */
static int hf_csmid_e1Content = -1;               /* INTEGER_128_16511 */
static int hf_csmid_e2Extension = -1;             /* Ext2 */
static int hf_csmid_e2Content = -1;               /* INTEGER_16512_2113663 */
static int hf_csmid_e1Extension = -1;             /* Ext3 */

/* --- Module EfcDsrcApplication --- --- ---                                  */

static int hf_dsrc_app_maxLadenweightOnAxle1 = -1;  /* Int2 */
static int hf_dsrc_app_maxLadenweightOnAxle2 = -1;  /* Int2 */
static int hf_dsrc_app_maxLadenweightOnAxle3 = -1;  /* Int2 */
static int hf_dsrc_app_maxLadenweightOnAxle4 = -1;  /* Int2 */
static int hf_dsrc_app_maxLadenweightOnAxle5 = -1;  /* Int2 */
static int hf_dsrc_app_particulate = -1;          /* T_particulate */
static int hf_dsrc_app_unitType = -1;             /* UnitType */
static int hf_dsrc_app_value = -1;                /* INTEGER_0_32767 */
static int hf_dsrc_app_absorptionCoeff = -1;      /* Int2 */
static int hf_dsrc_app_euroValue = -1;            /* EuroValue */
static int hf_dsrc_app_copValue = -1;             /* CopValue */
static int hf_dsrc_app_emissionCO = -1;           /* INTEGER_0_32767 */
static int hf_dsrc_app_emissionHC = -1;           /* Int2 */
static int hf_dsrc_app_emissionNOX = -1;          /* Int2 */
static int hf_dsrc_app_emissionHCNOX = -1;        /* Int2 */
static int hf_dsrc_app_numberOfSeats = -1;        /* Int1 */
static int hf_dsrc_app_numberOfStandingPlaces = -1;  /* Int1 */
static int hf_dsrc_app_countryCode = -1;          /* CountryCode */
static int hf_dsrc_app_providerIdentifier = -1;   /* IssuerIdentifier */
static int hf_dsrc_app_soundstationary = -1;      /* Int1 */
static int hf_dsrc_app_sounddriveby = -1;         /* Int1 */
static int hf_dsrc_app_vehicleLengthOverall = -1;  /* Int1 */
static int hf_dsrc_app_vehicleHeigthOverall = -1;  /* Int1 */
static int hf_dsrc_app_vehicleWidthOverall = -1;  /* Int1 */
static int hf_dsrc_app_vehicleMaxLadenWeight = -1;  /* Int2 */
static int hf_dsrc_app_vehicleTrainMaximumWeight = -1;  /* Int2 */
static int hf_dsrc_app_vehicleWeightUnladen = -1;  /* Int2 */

/* --- Module IVI --- --- ---                                                 */

static int hf_ivi_ivi_IviStructure_PDU = -1;      /* IviStructure */
static int hf_ivi_mandatory = -1;                 /* IVIManagementContainer */
static int hf_ivi_optional = -1;                  /* SEQUENCE_SIZE_1_8__OF_IviContainer */
static int hf_ivi_optional_item = -1;             /* IviContainer */
static int hf_ivi_glc = -1;                       /* GeographicLocationContainer */
static int hf_ivi_giv = -1;                       /* GeneralIviContainer */
static int hf_ivi_rcc = -1;                       /* RoadConfigurationContainer */
static int hf_ivi_tc = -1;                        /* TextContainer */
static int hf_ivi_lac = -1;                       /* LayoutContainer */
static int hf_ivi_serviceProviderId = -1;         /* Provider */
static int hf_ivi_iviIdentificationNumber = -1;   /* IviIdentificationNumber */
static int hf_ivi_timeStamp = -1;                 /* TimestampIts */
static int hf_ivi_validFrom = -1;                 /* TimestampIts */
static int hf_ivi_validTo = -1;                   /* TimestampIts */
static int hf_ivi_connectedIviStructures = -1;    /* SEQUENCE_SIZE_1_8_OF_IviIdentificationNumber */
static int hf_ivi_connectedIviStructures_item = -1;  /* IviIdentificationNumber */
static int hf_ivi_iviStatus = -1;                 /* IviStatus */
static int hf_ivi_referencePosition = -1;         /* ReferencePosition */
static int hf_ivi_referencePositionTime = -1;     /* TimestampIts */
static int hf_ivi_referencePositionHeading = -1;  /* Heading */
static int hf_ivi_referencePositionSpeed = -1;    /* Speed */
static int hf_ivi_parts = -1;                     /* SEQUENCE_SIZE_1_16__OF_GlcPart */
static int hf_ivi_parts_item = -1;                /* GlcPart */
static int hf_ivi_zoneId = -1;                    /* Zid */
static int hf_ivi_laneNumber = -1;                /* LanePosition */
static int hf_ivi_zoneExtension = -1;             /* INTEGER_0_255 */
static int hf_ivi_zoneHeading = -1;               /* HeadingValue */
static int hf_ivi_zone = -1;                      /* Zone */
static int hf_ivi_GeneralIviContainer_item = -1;  /* GicPart */
static int hf_ivi_gpDetectionZoneIds = -1;        /* T_GicPartDetectionZoneIds */
static int hf_ivi_gpDetectionZoneIds_item = -1;   /* Zid */
static int hf_ivi_its_Rrid = -1;                  /* VarLengthNumber */
static int hf_ivi_gpRelevanceZoneIds = -1;        /* T_GicPartRelevanceZoneIds */
static int hf_ivi_gpRelevanceZoneIds_item = -1;   /* Zid */
static int hf_ivi_direction = -1;                 /* Direction */
static int hf_ivi_gpDriverAwarenessZoneIds = -1;  /* T_GicPartDriverAwarenessZoneIds */
static int hf_ivi_gpDriverAwarenessZoneIds_item = -1;  /* Zid */
static int hf_ivi_minimumAwarenessTime = -1;      /* INTEGER_0_255 */
static int hf_ivi_applicableLanes = -1;           /* SEQUENCE_SIZE_1_8__OF_LanePosition */
static int hf_ivi_applicableLanes_item = -1;      /* LanePosition */
static int hf_ivi_iviType = -1;                   /* IviType */
static int hf_ivi_iviPurpose = -1;                /* IviPurpose */
static int hf_ivi_laneStatus = -1;                /* LaneStatus */
static int hf_ivi_vehicleCharacteristics = -1;    /* SEQUENCE_SIZE_1_8__OF_CompleteVehicleCharacteristics */
static int hf_ivi_vehicleCharacteristics_item = -1;  /* CompleteVehicleCharacteristics */
static int hf_ivi_driverCharacteristics = -1;     /* DriverCharacteristics */
static int hf_ivi_layoutId = -1;                  /* INTEGER_1_4_ */
static int hf_ivi_preStoredlayoutId = -1;         /* INTEGER_1_64_ */
static int hf_ivi_roadSignCodes = -1;             /* SEQUENCE_SIZE_1_4__OF_RSCode */
static int hf_ivi_roadSignCodes_item = -1;        /* RSCode */
static int hf_ivi_extraText = -1;                 /* T_GicPartExtraText */
static int hf_ivi_extraText_item = -1;            /* Text */
static int hf_ivi_RoadConfigurationContainer_item = -1;  /* RccPart */
static int hf_ivi_zoneIds = -1;                   /* SEQUENCE_SIZE_1_8__OF_Zid */
static int hf_ivi_zoneIds_item = -1;              /* Zid */
static int hf_ivi_roadType = -1;                  /* RoadType */
static int hf_ivi_laneConfiguration = -1;         /* SEQUENCE_SIZE_1_16__OF_LaneInformation */
static int hf_ivi_laneConfiguration_item = -1;    /* LaneInformation */
static int hf_ivi_TextContainer_item = -1;        /* TcPart */
static int hf_ivi_tpDetectionZoneIds = -1;        /* T_TcPartDetectionZoneIds */
static int hf_ivi_tpDetectionZoneIds_item = -1;   /* Zid */
static int hf_ivi_tpRelevanceZoneIds = -1;        /* T_TcPartRelevanceZoneIds */
static int hf_ivi_tpRelevanceZoneIds_item = -1;   /* Zid */
static int hf_ivi_tpDriverAwarenessZoneIds = -1;  /* T_TcPartDriverAwarenessZoneIds */
static int hf_ivi_tpDriverAwarenessZoneIds_item = -1;  /* Zid */
static int hf_ivi_text = -1;                      /* T_TcPartText */
static int hf_ivi_text_item = -1;                 /* Text */
static int hf_ivi_data = -1;                      /* OCTET_STRING */
static int hf_ivi_height = -1;                    /* INTEGER_10_73 */
static int hf_ivi_width = -1;                     /* INTEGER_10_265 */
static int hf_ivi_layoutComponents = -1;          /* SEQUENCE_SIZE_1_4__OF_LayoutComponent */
static int hf_ivi_layoutComponents_item = -1;     /* LayoutComponent */
static int hf_ivi_latitude = -1;                  /* Latitude */
static int hf_ivi_longitude = -1;                 /* Longitude */
static int hf_ivi_altitude = -1;                  /* Altitude */
static int hf_ivi_owner = -1;                     /* Provider */
static int hf_ivi_version = -1;                   /* INTEGER_0_255 */
static int hf_ivi_acPictogramCode = -1;           /* INTEGER_0_65535 */
static int hf_ivi_acValue = -1;                   /* INTEGER_0_65535 */
static int hf_ivi_unit = -1;                      /* RSCUnit */
static int hf_ivi_attributes = -1;                /* ISO14823Attributes */
static int hf_ivi_tractor = -1;                   /* TractorCharacteristics */
static int hf_ivi_trailer = -1;                   /* SEQUENCE_SIZE_1_3_OF_TrailerCharacteristics */
static int hf_ivi_trailer_item = -1;              /* TrailerCharacteristics */
static int hf_ivi_train = -1;                     /* TrainCharacteristics */
static int hf_ivi_laneWidth = -1;                 /* IVILaneWidth */
static int hf_ivi_offsetDistance = -1;            /* INTEGER_M32768_32767 */
static int hf_ivi_offsetPosition = -1;            /* DeltaReferencePosition */
static int hf_ivi_deltaLatitude = -1;             /* DeltaLatitude */
static int hf_ivi_deltaLongitude = -1;            /* DeltaLongitude */
static int hf_ivi_dValue = -1;                    /* INTEGER_1_16384 */
static int hf_ivi_dodValue = -1;                  /* INTEGER_1_16384 */
static int hf_ivi_ISO14823Attributes_item = -1;   /* ISO14823Attributes_item */
static int hf_ivi_dtm = -1;                       /* DTM */
static int hf_ivi_edt = -1;                       /* EDT */
static int hf_ivi_dfl = -1;                       /* DFL */
static int hf_ivi_ved = -1;                       /* VED */
static int hf_ivi_spe = -1;                       /* SPE */
static int hf_ivi_roi = -1;                       /* ROI */
static int hf_ivi_dbv = -1;                       /* DBV */
static int hf_ivi_ddd = -1;                       /* DDD */
static int hf_ivi_icPictogramCode = -1;           /* T_icPictogramCode */
static int hf_ivi_countryCode = -1;               /* OCTET_STRING_SIZE_2 */
static int hf_ivi_serviceCategoryCode = -1;       /* T_serviceCategoryCode */
static int hf_ivi_trafficSignPictogram = -1;      /* T_trafficSignPictogram */
static int hf_ivi_publicFacilitiesPictogram = -1;  /* T_publicFacilitiesPictogram */
static int hf_ivi_ambientOrRoadConditionPictogram = -1;  /* T_ambientOrRoadConditionPictogram */
static int hf_ivi_pictogramCategoryCode = -1;     /* T_pictogramCategoryCode */
static int hf_ivi_nature = -1;                    /* INTEGER_1_9 */
static int hf_ivi_serialNumber = -1;              /* INTEGER_0_99 */
static int hf_ivi_liValidity = -1;                /* DTM */
static int hf_ivi_laneType = -1;                  /* LaneType */
static int hf_ivi_laneTypeQualifier = -1;         /* CompleteVehicleCharacteristics */
static int hf_ivi_lcLayoutComponentId = -1;       /* INTEGER_1_8_ */
static int hf_ivi_x = -1;                         /* INTEGER_10_265 */
static int hf_ivi_y = -1;                         /* INTEGER_10_73 */
static int hf_ivi_textScripting = -1;             /* T_textScripting */
static int hf_ivi_goodsType = -1;                 /* GoodsType */
static int hf_ivi_dangerousGoodsType = -1;        /* DangerousGoodsBasic */
static int hf_ivi_specialTransportType = -1;      /* SpecialTransportType */
static int hf_ivi_deltaPositions = -1;            /* SEQUENCE_SIZE_1_32__OF_DeltaPosition */
static int hf_ivi_deltaPositions_item = -1;       /* DeltaPosition */
static int hf_ivi_deltaPositionsWithAltitude = -1;  /* SEQUENCE_SIZE_1_32__OF_DeltaReferencePosition */
static int hf_ivi_deltaPositionsWithAltitude_item = -1;  /* DeltaReferencePosition */
static int hf_ivi_absolutePositions = -1;         /* SEQUENCE_SIZE_1_8__OF_AbsolutePosition */
static int hf_ivi_absolutePositions_item = -1;    /* AbsolutePosition */
static int hf_ivi_absolutePositionsWithAltitude = -1;  /* SEQUENCE_SIZE_1_8__OF_AbsolutePositionWAltitude */
static int hf_ivi_absolutePositionsWithAltitude_item = -1;  /* AbsolutePositionWAltitude */
static int hf_ivi_rscLayoutComponentId = -1;      /* INTEGER_1_4_ */
static int hf_ivi_code = -1;                      /* T_code */
static int hf_ivi_viennaConvention = -1;          /* VcCode */
static int hf_ivi_iso14823 = -1;                  /* ISO14823Code */
static int hf_ivi_itisCodes = -1;                 /* INTEGER_0_65535 */
static int hf_ivi_anyCatalogue = -1;              /* AnyCatalogue */
static int hf_ivi_line = -1;                      /* PolygonalLine */
static int hf_ivi_tLayoutComponentId = -1;        /* INTEGER_1_4_ */
static int hf_ivi_language = -1;                  /* BIT_STRING_SIZE_10 */
static int hf_ivi_textContent = -1;               /* UTF8String */
static int hf_ivi_toEqualTo = -1;                 /* T_TractorCharactEqualTo */
static int hf_ivi_toEqualTo_item = -1;            /* VehicleCharacteristicsFixValues */
static int hf_ivi_toNotEqualTo = -1;              /* T_TractorCharactNotEqualTo */
static int hf_ivi_toNotEqualTo_item = -1;         /* VehicleCharacteristicsFixValues */
static int hf_ivi_ranges = -1;                    /* SEQUENCE_SIZE_1_4__OF_VehicleCharacteristicsRanges */
static int hf_ivi_ranges_item = -1;               /* VehicleCharacteristicsRanges */
static int hf_ivi_teEqualTo = -1;                 /* T_TrailerCharactEqualTo */
static int hf_ivi_teEqualTo_item = -1;            /* VehicleCharacteristicsFixValues */
static int hf_ivi_teNotEqualTo = -1;              /* T_TrailerCharactNotEqualTo */
static int hf_ivi_teNotEqualTo_item = -1;         /* VehicleCharacteristicsFixValues */
static int hf_ivi_roadSignClass = -1;             /* VcClass */
static int hf_ivi_roadSignCode = -1;              /* INTEGER_1_64 */
static int hf_ivi_vcOption = -1;                  /* VcOption */
static int hf_ivi_vcValidity = -1;                /* SEQUENCE_SIZE_1_8__OF_DTM */
static int hf_ivi_vcValidity_item = -1;           /* DTM */
static int hf_ivi_vcValue = -1;                   /* INTEGER_0_65535 */
static int hf_ivi_simpleVehicleType = -1;         /* StationType */
static int hf_ivi_euVehicleCategoryCode = -1;     /* EuVehicleCategoryCode */
static int hf_ivi_iso3833VehicleType = -1;        /* Iso3833VehicleType */
static int hf_ivi_euroAndCo2value = -1;           /* EnvironmentalCharacteristics */
static int hf_ivi_engineCharacteristics = -1;     /* EngineCharacteristics */
static int hf_ivi_loadType = -1;                  /* LoadType */
static int hf_ivi_usage = -1;                     /* VehicleRole */
static int hf_ivi_comparisonOperator = -1;        /* ComparisonOperator */
static int hf_ivi_limits = -1;                    /* T_limits */
static int hf_ivi_numberOfAxles = -1;             /* INTEGER_0_7 */
static int hf_ivi_vehicleDimensions = -1;         /* VehicleDimensions */
static int hf_ivi_vehicleWeightLimits = -1;       /* VehicleWeightLimits */
static int hf_ivi_axleWeightLimits = -1;          /* AxleWeightLimits */
static int hf_ivi_passengerCapacity = -1;         /* PassengerCapacity */
static int hf_ivi_exhaustEmissionValues = -1;     /* ExhaustEmissionValues */
static int hf_ivi_dieselEmissionValues = -1;      /* DieselEmissionValues */
static int hf_ivi_soundLevel = -1;                /* SoundLevel */
static int hf_ivi_wValue = -1;                    /* INTEGER_1_16384 */
static int hf_ivi_segment = -1;                   /* Segment */
static int hf_ivi_area = -1;                      /* PolygonalLine */
static int hf_ivi_computedSegment = -1;           /* ComputedSegment */
static int hf_ivi_year = -1;                      /* T_year */
static int hf_ivi_syr = -1;                       /* INTEGER_2000_2127_ */
static int hf_ivi_eyr = -1;                       /* INTEGER_2000_2127_ */
static int hf_ivi_month_day = -1;                 /* T_month_day */
static int hf_ivi_smd = -1;                       /* MonthDay */
static int hf_ivi_emd = -1;                       /* MonthDay */
static int hf_ivi_pmd = -1;                       /* PMD */
static int hf_ivi_hourMinutes = -1;               /* T_hourMinutes */
static int hf_ivi_shm = -1;                       /* HoursMinutes */
static int hf_ivi_ehm = -1;                       /* HoursMinutes */
static int hf_ivi_dayOfWeek = -1;                 /* DayOfWeek */
static int hf_ivi_period = -1;                    /* HoursMinutes */
static int hf_ivi_month = -1;                     /* INTEGER_1_12 */
static int hf_ivi_day = -1;                       /* INTEGER_1_31 */
static int hf_ivi_hours = -1;                     /* INTEGER_0_23 */
static int hf_ivi_mins = -1;                      /* INTEGER_0_59 */
static int hf_ivi_hei = -1;                       /* Distance */
static int hf_ivi_wid = -1;                       /* Distance */
static int hf_ivi_vln = -1;                       /* Distance */
static int hf_ivi_wei = -1;                       /* Weight */
static int hf_ivi_spm = -1;                       /* INTEGER_0_250 */
static int hf_ivi_mns = -1;                       /* INTEGER_0_250 */
static int hf_ivi_dcj = -1;                       /* INTEGER_1_128 */
static int hf_ivi_dcr = -1;                       /* INTEGER_1_128 */
static int hf_ivi_tpl = -1;                       /* INTEGER_1_128 */
static int hf_ivi_ioList = -1;                    /* SEQUENCE_SIZE_1_8__OF_DDD_IO */
static int hf_ivi_ioList_item = -1;               /* DDD_IO */
static int hf_ivi_drn = -1;                       /* INTEGER_0_7 */
static int hf_ivi_dp = -1;                        /* SEQUENCE_SIZE_1_4__OF_DestinationPlace */
static int hf_ivi_dp_item = -1;                   /* DestinationPlace */
static int hf_ivi_dr = -1;                        /* SEQUENCE_SIZE_1_4__OF_DestinationRoad */
static int hf_ivi_dr_item = -1;                   /* DestinationRoad */
static int hf_ivi_rne = -1;                       /* INTEGER_1_999 */
static int hf_ivi_stnId = -1;                     /* INTEGER_1_999 */
static int hf_ivi_stnText = -1;                   /* UTF8String */
static int hf_ivi_dcp = -1;                       /* DistanceOrDuration */
static int hf_ivi_ddp = -1;                       /* DistanceOrDuration */
static int hf_ivi_depType = -1;                   /* DDD_DEP */
static int hf_ivi_depRSCode = -1;                 /* ISO14823Code */
static int hf_ivi_depBlob = -1;                   /* OCTET_STRING */
static int hf_ivi_plnId = -1;                     /* INTEGER_1_999 */
static int hf_ivi_plnText = -1;                   /* UTF8String */
static int hf_ivi_derType = -1;                   /* DDD_DER */
static int hf_ivi_ronId = -1;                     /* INTEGER_1_999 */
static int hf_ivi_ronText = -1;                   /* UTF8String */
/* named bits */
static int hf_ivi_PMD_national_holiday = -1;
static int hf_ivi_PMD_even_days = -1;
static int hf_ivi_PMD_odd_days = -1;
static int hf_ivi_PMD_market_day = -1;
static int hf_ivi_DayOfWeek_unused = -1;
static int hf_ivi_DayOfWeek_monday = -1;
static int hf_ivi_DayOfWeek_tuesday = -1;
static int hf_ivi_DayOfWeek_wednesday = -1;
static int hf_ivi_DayOfWeek_thursday = -1;
static int hf_ivi_DayOfWeek_friday = -1;
static int hf_ivi_DayOfWeek_saturday = -1;
static int hf_ivi_DayOfWeek_sunday = -1;

/* --- Module DSRC --- --- ---                                                */

static int hf_dsrc_dsrc_SPAT_PDU = -1;            /* SPAT */
static int hf_dsrc_dsrc_SignalRequestMessage_PDU = -1;  /* SignalRequestMessage */
static int hf_dsrc_dsrc_SignalStatusMessage_PDU = -1;  /* SignalStatusMessage */
static int hf_dsrc_dsrc_MapData_PDU = -1;         /* MapData */
static int hf_dsrc_regionId = -1;                 /* RegionId */
static int hf_dsrc_regExtValue = -1;              /* T_regExtValue */
static int hf_dsrc_spatTimeStamp = -1;            /* MinuteOfTheYear */
static int hf_dsrc_name = -1;                     /* DescriptiveName */
static int hf_dsrc_spatIntersections = -1;        /* IntersectionStateList */
static int hf_dsrc_spatRegional = -1;             /* T_SPATRegional */
static int hf_dsrc_spatRegional_item = -1;        /* RegionalExtension */
static int hf_dsrc_srmTimeStamp = -1;             /* MinuteOfTheYear */
static int hf_dsrc_second = -1;                   /* DSecond */
static int hf_dsrc_sequenceNumber = -1;           /* MsgCount */
static int hf_dsrc_requests = -1;                 /* SignalRequestList */
static int hf_dsrc_requestor = -1;                /* RequestorDescription */
static int hf_dsrc_srmRegional = -1;              /* T_SRMRegional */
static int hf_dsrc_srmRegional_item = -1;         /* RegionalExtension */
static int hf_dsrc_ssmTimeStamp = -1;             /* MinuteOfTheYear */
static int hf_dsrc_ssmStatus = -1;                /* SignalStatusList */
static int hf_dsrc_ssmRegional = -1;              /* T_SSMRegional */
static int hf_dsrc_ssmRegional_item = -1;         /* RegionalExtension */
static int hf_dsrc_mdTimeStamp = -1;              /* MinuteOfTheYear */
static int hf_dsrc_msgIssueRevision = -1;         /* MsgCount */
static int hf_dsrc_layerType = -1;                /* LayerType */
static int hf_dsrc_layerID = -1;                  /* LayerID */
static int hf_dsrc_mdIntersections = -1;          /* IntersectionGeometryList */
static int hf_dsrc_roadSegments = -1;             /* RoadSegmentList */
static int hf_dsrc_dataParameters = -1;           /* DataParameters */
static int hf_dsrc_restrictionList = -1;          /* RestrictionClassList */
static int hf_dsrc_mapRegional = -1;              /* T_MAPRegional */
static int hf_dsrc_mapRegional_item = -1;         /* RegionalExtension */
static int hf_dsrc_asType = -1;                   /* AdvisorySpeedType */
static int hf_dsrc_asSpeed = -1;                  /* SpeedAdvice */
static int hf_dsrc_asConfidence = -1;             /* SpeedConfidence */
static int hf_dsrc_distance = -1;                 /* ZoneLength */
static int hf_dsrc_class = -1;                    /* RestrictionClassID */
static int hf_dsrc_asRegional = -1;               /* T_AdvisorySpeedRegional */
static int hf_dsrc_asRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_AdvisorySpeedList_item = -1;   /* AdvisorySpeed */
static int hf_dsrc_referenceLaneId = -1;          /* LaneID */
static int hf_dsrc_offsetXaxis = -1;              /* T_offsetXaxis */
static int hf_dsrc_small = -1;                    /* DrivenLineOffsetSm */
static int hf_dsrc_large = -1;                    /* DrivenLineOffsetLg */
static int hf_dsrc_offsetYaxis = -1;              /* T_offsetYaxis */
static int hf_dsrc_rotateXY = -1;                 /* Angle */
static int hf_dsrc_scaleXaxis = -1;               /* Scale_B12 */
static int hf_dsrc_scaleYaxis = -1;               /* Scale_B12 */
static int hf_dsrc_clRegional = -1;               /* T_ComputedLaneRegional */
static int hf_dsrc_clRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_ConnectsToList_item = -1;      /* Connection */
static int hf_dsrc_lane = -1;                     /* LaneID */
static int hf_dsrc_maneuver = -1;                 /* AllowedManeuvers */
static int hf_dsrc_connectingLane = -1;           /* ConnectingLane */
static int hf_dsrc_remoteIntersection = -1;       /* IntersectionReferenceID */
static int hf_dsrc_signalGroup = -1;              /* SignalGroupID */
static int hf_dsrc_userClass = -1;                /* RestrictionClassID */
static int hf_dsrc_connectionID = -1;             /* LaneConnectionID */
static int hf_dsrc_queueLength = -1;              /* ZoneLength */
static int hf_dsrc_availableStorageLength = -1;   /* ZoneLength */
static int hf_dsrc_waitOnStop = -1;               /* WaitOnStopline */
static int hf_dsrc_pedBicycleDetect = -1;         /* PedestrianBicycleDetect */
static int hf_dsrc_cmaRegional = -1;              /* T_ConnectionManeuverAssistRegional */
static int hf_dsrc_cmaRegional_item = -1;         /* RegionalExtension */
static int hf_dsrc_processMethod = -1;            /* IA5String_SIZE_1_255 */
static int hf_dsrc_processAgency = -1;            /* IA5String_SIZE_1_255 */
static int hf_dsrc_lastCheckedDate = -1;          /* IA5String_SIZE_1_255 */
static int hf_dsrc_geoidUsed = -1;                /* IA5String_SIZE_1_255 */
static int hf_dsrc_EnabledLaneList_item = -1;     /* LaneID */
static int hf_dsrc_laneID = -1;                   /* LaneID */
static int hf_dsrc_ingressApproach = -1;          /* ApproachID */
static int hf_dsrc_egressApproach = -1;           /* ApproachID */
static int hf_dsrc_laneAttributes = -1;           /* LaneAttributes */
static int hf_dsrc_maneuvers = -1;                /* AllowedManeuvers */
static int hf_dsrc_nodeList = -1;                 /* NodeListXY */
static int hf_dsrc_connectsTo = -1;               /* ConnectsToList */
static int hf_dsrc_overlays = -1;                 /* OverlayLaneList */
static int hf_dsrc_glRegional = -1;               /* T_GenericLaneRegional */
static int hf_dsrc_glRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_approach = -1;                 /* ApproachID */
static int hf_dsrc_connection = -1;               /* LaneConnectionID */
static int hf_dsrc_igId = -1;                     /* IntersectionReferenceID */
static int hf_dsrc_revision = -1;                 /* MsgCount */
static int hf_dsrc_refPoint = -1;                 /* Position3D */
static int hf_dsrc_laneWidth = -1;                /* LaneWidth */
static int hf_dsrc_speedLimits = -1;              /* SpeedLimitList */
static int hf_dsrc_laneSet = -1;                  /* LaneList */
static int hf_dsrc_preemptPriorityData = -1;      /* PreemptPriorityList */
static int hf_dsrc_igRegional = -1;               /* T_IntersectionGeometryRegional */
static int hf_dsrc_igRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_IntersectionGeometryList_item = -1;  /* IntersectionGeometry */
static int hf_dsrc_region = -1;                   /* RoadRegulatorID */
static int hf_dsrc_irId = -1;                     /* IntersectionID */
static int hf_dsrc_isId = -1;                     /* IntersectionReferenceID */
static int hf_dsrc_isStatus = -1;                 /* IntersectionStatusObject */
static int hf_dsrc_moy = -1;                      /* MinuteOfTheYear */
static int hf_dsrc_isTimeStamp = -1;              /* DSecond */
static int hf_dsrc_enabledLanes = -1;             /* EnabledLaneList */
static int hf_dsrc_states = -1;                   /* MovementList */
static int hf_dsrc_maneuverAssistList = -1;       /* ManeuverAssistList */
static int hf_dsrc_isRegional = -1;               /* T_IntersectionStateRegional */
static int hf_dsrc_isRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_IntersectionStateList_item = -1;  /* IntersectionState */
static int hf_dsrc_directionalUse = -1;           /* LaneDirection */
static int hf_dsrc_sharedWith = -1;               /* LaneSharing */
static int hf_dsrc_laneType = -1;                 /* LaneTypeAttributes */
static int hf_dsrc_laRegional = -1;               /* RegionalExtension */
static int hf_dsrc_pathEndPointAngle = -1;        /* DeltaAngle */
static int hf_dsrc_laneCrownPointCenter = -1;     /* RoadwayCrownAngle */
static int hf_dsrc_laneCrownPointLeft = -1;       /* RoadwayCrownAngle */
static int hf_dsrc_laneCrownPointRight = -1;      /* RoadwayCrownAngle */
static int hf_dsrc_laneAngle = -1;                /* MergeDivergeNodeAngle */
static int hf_dsrc_ldaRegional = -1;              /* T_LaneDataAttributeRegional */
static int hf_dsrc_ldaRegional_item = -1;         /* RegionalExtension */
static int hf_dsrc_LaneDataAttributeList_item = -1;  /* LaneDataAttribute */
static int hf_dsrc_LaneList_item = -1;            /* GenericLane */
static int hf_dsrc_vehicle = -1;                  /* LaneAttributes_Vehicle */
static int hf_dsrc_crosswalk = -1;                /* LaneAttributes_Crosswalk */
static int hf_dsrc_bikeLane = -1;                 /* LaneAttributes_Bike */
static int hf_dsrc_sidewalk = -1;                 /* LaneAttributes_Sidewalk */
static int hf_dsrc_median = -1;                   /* LaneAttributes_Barrier */
static int hf_dsrc_striping = -1;                 /* LaneAttributes_Striping */
static int hf_dsrc_trackedVehicle = -1;           /* LaneAttributes_TrackedVehicle */
static int hf_dsrc_parking = -1;                  /* LaneAttributes_Parking */
static int hf_dsrc_ManeuverAssistList_item = -1;  /* ConnectionManeuverAssist */
static int hf_dsrc_MovementEventList_item = -1;   /* MovementEvent */
static int hf_dsrc_eventState = -1;               /* MovementPhaseState */
static int hf_dsrc_timing = -1;                   /* TimeChangeDetails */
static int hf_dsrc_speeds = -1;                   /* AdvisorySpeedList */
static int hf_dsrc_meRegional = -1;               /* T_MovementEventRegional */
static int hf_dsrc_meRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_MovementList_item = -1;        /* MovementState */
static int hf_dsrc_movementName = -1;             /* DescriptiveName */
static int hf_dsrc_state_time_speed = -1;         /* MovementEventList */
static int hf_dsrc_msRegional = -1;               /* T_MovementStateRegional */
static int hf_dsrc_msRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_localNode = -1;                /* NodeAttributeXYList */
static int hf_dsrc_disabled = -1;                 /* SegmentAttributeXYList */
static int hf_dsrc_enabled = -1;                  /* SegmentAttributeXYList */
static int hf_dsrc_data = -1;                     /* LaneDataAttributeList */
static int hf_dsrc_dWidth = -1;                   /* Offset_B10 */
static int hf_dsrc_dElevation = -1;               /* Offset_B10 */
static int hf_dsrc_nasxyRegional = -1;            /* T_NodeAttributeSetXYRegional */
static int hf_dsrc_nasxyRegional_item = -1;       /* RegionalExtension */
static int hf_dsrc_NodeAttributeXYList_item = -1;  /* NodeAttributeXY */
static int hf_dsrc_lon = -1;                      /* Longitude */
static int hf_dsrc_lat = -1;                      /* Latitude */
static int hf_dsrc_n20bX = -1;                    /* Offset_B10 */
static int hf_dsrc_n20bY = -1;                    /* Offset_B10 */
static int hf_dsrc_n22bX = -1;                    /* Offset_B11 */
static int hf_dsrc_n22bY = -1;                    /* Offset_B11 */
static int hf_dsrc_n24bX = -1;                    /* Offset_B12 */
static int hf_dsrc_n24bY = -1;                    /* Offset_B12 */
static int hf_dsrc_n26bX = -1;                    /* Offset_B13 */
static int hf_dsrc_n26bY = -1;                    /* Offset_B13 */
static int hf_dsrc_n28bX = -1;                    /* Offset_B14 */
static int hf_dsrc_n28bY = -1;                    /* Offset_B14 */
static int hf_dsrc_n32bX = -1;                    /* Offset_B16 */
static int hf_dsrc_n32bY = -1;                    /* Offset_B16 */
static int hf_dsrc_nodes = -1;                    /* NodeSetXY */
static int hf_dsrc_computed = -1;                 /* ComputedLane */
static int hf_dsrc_node_XY1 = -1;                 /* Node_XY_20b */
static int hf_dsrc_node_XY2 = -1;                 /* Node_XY_22b */
static int hf_dsrc_node_XY3 = -1;                 /* Node_XY_24b */
static int hf_dsrc_node_XY4 = -1;                 /* Node_XY_26b */
static int hf_dsrc_node_XY5 = -1;                 /* Node_XY_28b */
static int hf_dsrc_node_XY6 = -1;                 /* Node_XY_32b */
static int hf_dsrc_node_LatLon = -1;              /* Node_LLmD_64b */
static int hf_dsrc_nopxyRegional = -1;            /* RegionalExtension */
static int hf_dsrc_NodeSetXY_item = -1;           /* NodeXY */
static int hf_dsrc_delta = -1;                    /* NodeOffsetPointXY */
static int hf_dsrc_attributes = -1;               /* NodeAttributeSetXY */
static int hf_dsrc_OverlayLaneList_item = -1;     /* LaneID */
static int hf_dsrc_long = -1;                     /* Longitude */
static int hf_dsrc_elevation = -1;                /* Elevation */
static int hf_dsrc_p3dRegional = -1;              /* T_Position3DRegional */
static int hf_dsrc_p3dRegional_item = -1;         /* RegionalExtension */
static int hf_dsrc_PreemptPriorityList_item = -1;  /* SignalControlZone */
static int hf_dsrc_rslType = -1;                  /* SpeedLimitType */
static int hf_dsrc_rslSpeed = -1;                 /* Velocity */
static int hf_dsrc_rdId = -1;                     /* VehicleID */
static int hf_dsrc_rdType = -1;                   /* RequestorType */
static int hf_dsrc_rdPosition = -1;               /* RequestorPositionVector */
static int hf_dsrc_routeName = -1;                /* DescriptiveName */
static int hf_dsrc_transitStatus = -1;            /* TransitVehicleStatus */
static int hf_dsrc_transitOccupancy = -1;         /* TransitVehicleOccupancy */
static int hf_dsrc_transitSchedule = -1;          /* DeltaTime */
static int hf_dsrc_rdRegional = -1;               /* T_RequestorDescriptionRegional */
static int hf_dsrc_rdRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_rpvPosition = -1;              /* Position3D */
static int hf_dsrc_heading = -1;                  /* Angle */
static int hf_dsrc_rpvSpeed = -1;                 /* TransmissionAndSpeed */
static int hf_dsrc_role = -1;                     /* BasicVehicleRole */
static int hf_dsrc_subrole = -1;                  /* RequestSubRole */
static int hf_dsrc_rtRequest = -1;                /* RequestImportanceLevel */
static int hf_dsrc_iso3883 = -1;                  /* Iso3833VehicleType */
static int hf_dsrc_hpmsType = -1;                 /* VehicleType */
static int hf_dsrc_rtRegional = -1;               /* RegionalExtension */
static int hf_dsrc_scaId = -1;                    /* RestrictionClassID */
static int hf_dsrc_users = -1;                    /* RestrictionUserTypeList */
static int hf_dsrc_RestrictionClassList_item = -1;  /* RestrictionClassAssignment */
static int hf_dsrc_RestrictionUserTypeList_item = -1;  /* RestrictionUserType */
static int hf_dsrc_basicType = -1;                /* RestrictionAppliesTo */
static int hf_dsrc_rutRegional = -1;              /* T_RestrictionUserTypeRegional */
static int hf_dsrc_rutRegional_item = -1;         /* RegionalExtension */
static int hf_dsrc_RoadLaneSetList_item = -1;     /* GenericLane */
static int hf_dsrc_rsrId = -1;                    /* RoadSegmentID */
static int hf_dsrc_rsId = -1;                     /* RoadSegmentReferenceID */
static int hf_dsrc_roadLaneSet = -1;              /* RoadLaneSetList */
static int hf_dsrc_rsRegional = -1;               /* T_RoadSegmentRegional */
static int hf_dsrc_rsRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_RoadSegmentList_item = -1;     /* RoadSegment */
static int hf_dsrc_SegmentAttributeXYList_item = -1;  /* SegmentAttributeXY */
static int hf_dsrc_zone = -1;                     /* RegionalExtension */
static int hf_dsrc_sriId = -1;                    /* VehicleID */
static int hf_dsrc_sriRequest = -1;               /* RequestID */
static int hf_dsrc_typeData = -1;                 /* RequestorType */
static int hf_dsrc_SignalRequestList_item = -1;   /* SignalRequestPackage */
static int hf_dsrc_srpRequest = -1;               /* SignalRequest */
static int hf_dsrc_minute = -1;                   /* MinuteOfTheYear */
static int hf_dsrc_duration = -1;                 /* DSecond */
static int hf_dsrc_srpRegional = -1;              /* T_SignalRequestPackageRegional */
static int hf_dsrc_srpRegional_item = -1;         /* RegionalExtension */
static int hf_dsrc_srId = -1;                     /* IntersectionReferenceID */
static int hf_dsrc_requestID = -1;                /* RequestID */
static int hf_dsrc_requestType = -1;              /* PriorityRequestType */
static int hf_dsrc_inBoundLane = -1;              /* IntersectionAccessPoint */
static int hf_dsrc_outBoundLane = -1;             /* IntersectionAccessPoint */
static int hf_dsrc_srRegional = -1;               /* T_SignalRequestRegional */
static int hf_dsrc_srRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_SignalStatusList_item = -1;    /* SignalStatus */
static int hf_dsrc_SignalStatusPackageList_item = -1;  /* SignalStatusPackage */
static int hf_dsrc_requester = -1;                /* SignalRequesterInfo */
static int hf_dsrc_inboundOn = -1;                /* IntersectionAccessPoint */
static int hf_dsrc_outboundOn = -1;               /* IntersectionAccessPoint */
static int hf_dsrc_sspStatus = -1;                /* PrioritizationResponseStatus */
static int hf_dsrc_sspRegional = -1;              /* T_SignalStatusPackageRegional */
static int hf_dsrc_sspRegional_item = -1;         /* RegionalExtension */
static int hf_dsrc_ssId = -1;                     /* IntersectionReferenceID */
static int hf_dsrc_sigStatus = -1;                /* SignalStatusPackageList */
static int hf_dsrc_ssRegional = -1;               /* T_SignalStatusRegional */
static int hf_dsrc_ssRegional_item = -1;          /* RegionalExtension */
static int hf_dsrc_SpeedLimitList_item = -1;      /* RegulatorySpeedLimit */
static int hf_dsrc_startTime = -1;                /* TimeMark */
static int hf_dsrc_minEndTime = -1;               /* TimeMark */
static int hf_dsrc_maxEndTime = -1;               /* TimeMark */
static int hf_dsrc_likelyTime = -1;               /* TimeMark */
static int hf_dsrc_tcdConfidence = -1;            /* TimeIntervalConfidence */
static int hf_dsrc_nextTime = -1;                 /* TimeMark */
static int hf_dsrc_transmisson = -1;              /* TransmissionState */
static int hf_dsrc_tasSpeed = -1;                 /* Velocity */
static int hf_dsrc_entityID = -1;                 /* TemporaryID */
static int hf_dsrc_stationID = -1;                /* StationID */
/* named bits */
static int hf_dsrc_LaneSharing_overlappingLaneDescriptionProvided = -1;
static int hf_dsrc_LaneSharing_multipleLanesTreatedAsOneLane = -1;
static int hf_dsrc_LaneSharing_otherNonMotorizedTrafficTypes = -1;
static int hf_dsrc_LaneSharing_individualMotorizedVehicleTraffic = -1;
static int hf_dsrc_LaneSharing_busVehicleTraffic = -1;
static int hf_dsrc_LaneSharing_taxiVehicleTraffic = -1;
static int hf_dsrc_LaneSharing_pedestriansTraffic = -1;
static int hf_dsrc_LaneSharing_cyclistVehicleTraffic = -1;
static int hf_dsrc_LaneSharing_trackedVehicleTraffic = -1;
static int hf_dsrc_LaneSharing_pedestrianTraffic = -1;
static int hf_dsrc_AllowedManeuvers_maneuverStraightAllowed = -1;
static int hf_dsrc_AllowedManeuvers_maneuverLeftAllowed = -1;
static int hf_dsrc_AllowedManeuvers_maneuverRightAllowed = -1;
static int hf_dsrc_AllowedManeuvers_maneuverUTurnAllowed = -1;
static int hf_dsrc_AllowedManeuvers_maneuverLeftTurnOnRedAllowed = -1;
static int hf_dsrc_AllowedManeuvers_maneuverRightTurnOnRedAllowed = -1;
static int hf_dsrc_AllowedManeuvers_maneuverLaneChangeAllowed = -1;
static int hf_dsrc_AllowedManeuvers_maneuverNoStoppingAllowed = -1;
static int hf_dsrc_AllowedManeuvers_yieldAllwaysRequired = -1;
static int hf_dsrc_AllowedManeuvers_goWithHalt = -1;
static int hf_dsrc_AllowedManeuvers_caution = -1;
static int hf_dsrc_AllowedManeuvers_reserved1 = -1;
static int hf_dsrc_IntersectionStatusObject_manualControlIsEnabled = -1;
static int hf_dsrc_IntersectionStatusObject_stopTimeIsActivated = -1;
static int hf_dsrc_IntersectionStatusObject_failureFlash = -1;
static int hf_dsrc_IntersectionStatusObject_preemptIsActive = -1;
static int hf_dsrc_IntersectionStatusObject_signalPriorityIsActive = -1;
static int hf_dsrc_IntersectionStatusObject_fixedTimeOperation = -1;
static int hf_dsrc_IntersectionStatusObject_trafficDependentOperation = -1;
static int hf_dsrc_IntersectionStatusObject_standbyOperation = -1;
static int hf_dsrc_IntersectionStatusObject_failureMode = -1;
static int hf_dsrc_IntersectionStatusObject_off = -1;
static int hf_dsrc_IntersectionStatusObject_recentMAPmessageUpdate = -1;
static int hf_dsrc_IntersectionStatusObject_recentChangeInMAPassignedLanesIDsUsed = -1;
static int hf_dsrc_IntersectionStatusObject_noValidMAPisAvailableAtThisTime = -1;
static int hf_dsrc_IntersectionStatusObject_noValidSPATisAvailableAtThisTime = -1;
static int hf_dsrc_LaneAttributes_Barrier_median_RevocableLane = -1;
static int hf_dsrc_LaneAttributes_Barrier_median = -1;
static int hf_dsrc_LaneAttributes_Barrier_whiteLineHashing = -1;
static int hf_dsrc_LaneAttributes_Barrier_stripedLines = -1;
static int hf_dsrc_LaneAttributes_Barrier_doubleStripedLines = -1;
static int hf_dsrc_LaneAttributes_Barrier_trafficCones = -1;
static int hf_dsrc_LaneAttributes_Barrier_constructionBarrier = -1;
static int hf_dsrc_LaneAttributes_Barrier_trafficChannels = -1;
static int hf_dsrc_LaneAttributes_Barrier_lowCurbs = -1;
static int hf_dsrc_LaneAttributes_Barrier_highCurbs = -1;
static int hf_dsrc_LaneAttributes_Bike_bikeRevocableLane = -1;
static int hf_dsrc_LaneAttributes_Bike_pedestrianUseAllowed = -1;
static int hf_dsrc_LaneAttributes_Bike_isBikeFlyOverLane = -1;
static int hf_dsrc_LaneAttributes_Bike_fixedCycleTime = -1;
static int hf_dsrc_LaneAttributes_Bike_biDirectionalCycleTimes = -1;
static int hf_dsrc_LaneAttributes_Bike_isolatedByBarrier = -1;
static int hf_dsrc_LaneAttributes_Bike_unsignalizedSegmentsPresent = -1;
static int hf_dsrc_LaneAttributes_Crosswalk_crosswalkRevocableLane = -1;
static int hf_dsrc_LaneAttributes_Crosswalk_bicyleUseAllowed = -1;
static int hf_dsrc_LaneAttributes_Crosswalk_isXwalkFlyOverLane = -1;
static int hf_dsrc_LaneAttributes_Crosswalk_fixedCycleTime = -1;
static int hf_dsrc_LaneAttributes_Crosswalk_biDirectionalCycleTimes = -1;
static int hf_dsrc_LaneAttributes_Crosswalk_hasPushToWalkButton = -1;
static int hf_dsrc_LaneAttributes_Crosswalk_audioSupport = -1;
static int hf_dsrc_LaneAttributes_Crosswalk_rfSignalRequestPresent = -1;
static int hf_dsrc_LaneAttributes_Crosswalk_unsignalizedSegmentsPresent = -1;
static int hf_dsrc_LaneAttributes_Parking_parkingRevocableLane = -1;
static int hf_dsrc_LaneAttributes_Parking_parallelParkingInUse = -1;
static int hf_dsrc_LaneAttributes_Parking_headInParkingInUse = -1;
static int hf_dsrc_LaneAttributes_Parking_doNotParkZone = -1;
static int hf_dsrc_LaneAttributes_Parking_parkingForBusUse = -1;
static int hf_dsrc_LaneAttributes_Parking_parkingForTaxiUse = -1;
static int hf_dsrc_LaneAttributes_Parking_noPublicParkingUse = -1;
static int hf_dsrc_LaneAttributes_Sidewalk_sidewalk_RevocableLane = -1;
static int hf_dsrc_LaneAttributes_Sidewalk_bicyleUseAllowed = -1;
static int hf_dsrc_LaneAttributes_Sidewalk_isSidewalkFlyOverLane = -1;
static int hf_dsrc_LaneAttributes_Sidewalk_walkBikes = -1;
static int hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesRevocableLane = -1;
static int hf_dsrc_LaneAttributes_Striping_stripeDrawOnLeft = -1;
static int hf_dsrc_LaneAttributes_Striping_stripeDrawOnRight = -1;
static int hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesLeft = -1;
static int hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesRight = -1;
static int hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesAhead = -1;
static int hf_dsrc_LaneAttributes_TrackedVehicle_spec_RevocableLane = -1;
static int hf_dsrc_LaneAttributes_TrackedVehicle_spec_commuterRailRoadTrack = -1;
static int hf_dsrc_LaneAttributes_TrackedVehicle_spec_lightRailRoadTrack = -1;
static int hf_dsrc_LaneAttributes_TrackedVehicle_spec_heavyRailRoadTrack = -1;
static int hf_dsrc_LaneAttributes_TrackedVehicle_spec_otherRailType = -1;
static int hf_dsrc_LaneAttributes_Vehicle_isVehicleRevocableLane = -1;
static int hf_dsrc_LaneAttributes_Vehicle_isVehicleFlyOverLane = -1;
static int hf_dsrc_LaneAttributes_Vehicle_hovLaneUseOnly = -1;
static int hf_dsrc_LaneAttributes_Vehicle_restrictedToBusUse = -1;
static int hf_dsrc_LaneAttributes_Vehicle_restrictedToTaxiUse = -1;
static int hf_dsrc_LaneAttributes_Vehicle_restrictedFromPublicUse = -1;
static int hf_dsrc_LaneAttributes_Vehicle_hasIRbeaconCoverage = -1;
static int hf_dsrc_LaneAttributes_Vehicle_permissionOnRequest = -1;
static int hf_dsrc_LaneDirection_ingressPath = -1;
static int hf_dsrc_LaneDirection_egressPath = -1;
static int hf_dsrc_TransitVehicleStatus_loading = -1;
static int hf_dsrc_TransitVehicleStatus_anADAuse = -1;
static int hf_dsrc_TransitVehicleStatus_aBikeLoad = -1;
static int hf_dsrc_TransitVehicleStatus_doorOpen = -1;
static int hf_dsrc_TransitVehicleStatus_charging = -1;
static int hf_dsrc_TransitVehicleStatus_atStopLine = -1;

/* --- Module AddGrpC --- --- ---                                             */

static int hf_AddGrpC_AddGrpC_ConnectionManeuverAssist_addGrpC_PDU = -1;  /* ConnectionManeuverAssist_addGrpC */
static int hf_AddGrpC_AddGrpC_ConnectionTrajectory_addGrpC_PDU = -1;  /* ConnectionTrajectory_addGrpC */
static int hf_AddGrpC_AddGrpC_Control_addGrpC_PDU = -1;  /* Control_addGrpC */
static int hf_AddGrpC_AddGrpC_IntersectionState_addGrpC_PDU = -1;  /* IntersectionState_addGrpC */
static int hf_AddGrpC_AddGrpC_MapData_addGrpC_PDU = -1;  /* MapData_addGrpC */
static int hf_AddGrpC_AddGrpC_Position3D_addGrpC_PDU = -1;  /* Position3D_addGrpC */
static int hf_AddGrpC_AddGrpC_RestrictionUserType_addGrpC_PDU = -1;  /* RestrictionUserType_addGrpC */
static int hf_AddGrpC_AddGrpC_SignalStatusPackage_addGrpC_PDU = -1;  /* SignalStatusPackage_addGrpC */
static int hf_AddGrpC_itsStationPositions = -1;   /* ItsStationPositionList */
static int hf_AddGrpC_rsuGNSSOffset = -1;         /* NodeOffsetPointXY */
static int hf_AddGrpC_nodes = -1;                 /* NodeSetXY */
static int hf_AddGrpC_ptvRequest = -1;            /* PtvRequestType */
static int hf_AddGrpC_activePrioritizations = -1;  /* PrioritizationResponseList */
static int hf_AddGrpC_signalHeadLocations = -1;   /* SignalHeadLocationList */
static int hf_AddGrpC_altitude = -1;              /* Altitude */
static int hf_AddGrpC_PrioritizationResponseList_item = -1;  /* PrioritizationResponse */
static int hf_AddGrpC_stationID = -1;             /* StationID */
static int hf_AddGrpC_priorState = -1;            /* PrioritizationResponseStatus */
static int hf_AddGrpC_signalGroup = -1;           /* SignalGroupID */
static int hf_AddGrpC_emission = -1;              /* EmissionType */
static int hf_AddGrpC_SignalHeadLocationList_item = -1;  /* SignalHeadLocation */
static int hf_AddGrpC_nodeXY = -1;                /* NodeOffsetPointXY */
static int hf_AddGrpC_nodeZ = -1;                 /* DeltaAltitude */
static int hf_AddGrpC_signalGroupID = -1;         /* SignalGroupID */
static int hf_AddGrpC_synchToSchedule = -1;       /* DeltaTime */
static int hf_AddGrpC_ItsStationPositionList_item = -1;  /* ItsStationPosition */
static int hf_AddGrpC_laneID = -1;                /* LaneID */
static int hf_AddGrpC_timeReference = -1;         /* TimeReference */

/* --- Module CAM-PDU-Descriptions --- --- ---                                */

static int hf_cam_cam_CoopAwareness_PDU = -1;     /* CoopAwareness */
static int hf_cam_generationDeltaTime = -1;       /* GenerationDeltaTime */
static int hf_cam_camParameters = -1;             /* CamParameters */
static int hf_cam_basicContainer = -1;            /* BasicContainer */
static int hf_cam_highFrequencyContainer = -1;    /* HighFrequencyContainer */
static int hf_cam_lowFrequencyContainer = -1;     /* LowFrequencyContainer */
static int hf_cam_specialVehicleContainer = -1;   /* SpecialVehicleContainer */
static int hf_cam_basicVehicleContainerHighFrequency = -1;  /* BasicVehicleContainerHighFrequency */
static int hf_cam_rsuContainerHighFrequency = -1;  /* RSUContainerHighFrequency */
static int hf_cam_basicVehicleContainerLowFrequency = -1;  /* BasicVehicleContainerLowFrequency */
static int hf_cam_publicTransportContainer = -1;  /* PublicTransportContainer */
static int hf_cam_specialTransportContainer = -1;  /* SpecialTransportContainer */
static int hf_cam_dangerousGoodsContainer = -1;   /* DangerousGoodsContainer */
static int hf_cam_roadWorksContainerBasic = -1;   /* RoadWorksContainerBasic */
static int hf_cam_rescueContainer = -1;           /* RescueContainer */
static int hf_cam_emergencyContainer = -1;        /* EmergencyContainer */
static int hf_cam_safetyCarContainer = -1;        /* SafetyCarContainer */
static int hf_cam_stationType = -1;               /* StationType */
static int hf_cam_referencePosition = -1;         /* ReferencePosition */
static int hf_cam_heading = -1;                   /* Heading */
static int hf_cam_speed = -1;                     /* Speed */
static int hf_cam_driveDirection = -1;            /* DriveDirection */
static int hf_cam_vehicleLength = -1;             /* VehicleLength */
static int hf_cam_vehicleWidth = -1;              /* VehicleWidth */
static int hf_cam_longitudinalAcceleration = -1;  /* LongitudinalAcceleration */
static int hf_cam_curvature = -1;                 /* Curvature */
static int hf_cam_curvatureCalculationMode = -1;  /* CurvatureCalculationMode */
static int hf_cam_yawRate = -1;                   /* YawRate */
static int hf_cam_accelerationControl = -1;       /* AccelerationControl */
static int hf_cam_lanePosition = -1;              /* LanePosition */
static int hf_cam_steeringWheelAngle = -1;        /* SteeringWheelAngle */
static int hf_cam_lateralAcceleration = -1;       /* LateralAcceleration */
static int hf_cam_verticalAcceleration = -1;      /* VerticalAcceleration */
static int hf_cam_performanceClass = -1;          /* PerformanceClass */
static int hf_cam_cenDsrcTollingZone = -1;        /* CenDsrcTollingZone */
static int hf_cam_vehicleRole = -1;               /* VehicleRole */
static int hf_cam_exteriorLights = -1;            /* ExteriorLights */
static int hf_cam_pathHistory = -1;               /* PathHistory */
static int hf_cam_embarkationStatus = -1;         /* EmbarkationStatus */
static int hf_cam_ptActivation = -1;              /* PtActivation */
static int hf_cam_specialTransportType = -1;      /* SpecialTransportType */
static int hf_cam_lightBarSirenInUse = -1;        /* LightBarSirenInUse */
static int hf_cam_dangerousGoodsBasic = -1;       /* DangerousGoodsBasic */
static int hf_cam_roadworksSubCauseCode = -1;     /* RoadworksSubCauseCode */
static int hf_cam_closedLanes = -1;               /* ClosedLanes */
static int hf_cam_incidentIndication = -1;        /* CauseCode */
static int hf_cam_emergencyPriority = -1;         /* EmergencyPriority */
static int hf_cam_trafficRule = -1;               /* TrafficRule */
static int hf_cam_speedLimit = -1;                /* SpeedLimit */
static int hf_cam_protectedCommunicationZonesRSU = -1;  /* ProtectedCommunicationZonesRSU */

/* --- Module DENM-PDU-Descriptions --- --- ---                               */

static int hf_denm_denm_DecentralizedEnvironmentalNotificationMessage_PDU = -1;  /* DecentralizedEnvironmentalNotificationMessage */
static int hf_denm_management = -1;               /* ManagementContainer */
static int hf_denm_situation = -1;                /* SituationContainer */
static int hf_denm_location = -1;                 /* LocationContainer */
static int hf_denm_alacarte = -1;                 /* AlacarteContainer */
static int hf_denm_actionID = -1;                 /* ActionID */
static int hf_denm_detectionTime = -1;            /* TimestampIts */
static int hf_denm_referenceTime = -1;            /* TimestampIts */
static int hf_denm_termination = -1;              /* Termination */
static int hf_denm_eventPosition = -1;            /* ReferencePosition */
static int hf_denm_relevanceDistance = -1;        /* RelevanceDistance */
static int hf_denm_relevanceTrafficDirection = -1;  /* RelevanceTrafficDirection */
static int hf_denm_validityDuration = -1;         /* ValidityDuration */
static int hf_denm_transmissionInterval = -1;     /* TransmissionInterval */
static int hf_denm_stationType = -1;              /* StationType */
static int hf_denm_informationQuality = -1;       /* InformationQuality */
static int hf_denm_eventType = -1;                /* CauseCode */
static int hf_denm_linkedCause = -1;              /* CauseCode */
static int hf_denm_eventHistory = -1;             /* EventHistory */
static int hf_denm_eventSpeed = -1;               /* Speed */
static int hf_denm_eventPositionHeading = -1;     /* Heading */
static int hf_denm_traces = -1;                   /* Traces */
static int hf_denm_roadType = -1;                 /* RoadType */
static int hf_denm_heightLonCarrLeft = -1;        /* HeightLonCarr */
static int hf_denm_heightLonCarrRight = -1;       /* HeightLonCarr */
static int hf_denm_posLonCarrLeft = -1;           /* PosLonCarr */
static int hf_denm_posLonCarrRight = -1;          /* PosLonCarr */
static int hf_denm_positionOfPillars = -1;        /* PositionOfPillars */
static int hf_denm_posCentMass = -1;              /* PosCentMass */
static int hf_denm_wheelBaseVehicle = -1;         /* WheelBaseVehicle */
static int hf_denm_turningRadius = -1;            /* TurningRadius */
static int hf_denm_posFrontAx = -1;               /* PosFrontAx */
static int hf_denm_positionOfOccupants = -1;      /* PositionOfOccupants */
static int hf_denm_vehicleMass = -1;              /* VehicleMass */
static int hf_denm_requestResponseIndication = -1;  /* RequestResponseIndication */
static int hf_denm_lightBarSirenInUse = -1;       /* LightBarSirenInUse */
static int hf_denm_closedLanes = -1;              /* ClosedLanes */
static int hf_denm_restriction = -1;              /* RestrictedTypes */
static int hf_denm_speedLimit = -1;               /* SpeedLimit */
static int hf_denm_incidentIndication = -1;       /* CauseCode */
static int hf_denm_recommendedPath = -1;          /* ItineraryPath */
static int hf_denm_startingPointSpeedLimit = -1;  /* DeltaReferencePosition */
static int hf_denm_trafficFlowRule = -1;          /* TrafficRule */
static int hf_denm_referenceDenms = -1;           /* ReferenceDenms */
static int hf_denm_stationarySince = -1;          /* StationarySince */
static int hf_denm_stationaryCause = -1;          /* CauseCode */
static int hf_denm_carryingDangerousGoods = -1;   /* DangerousGoodsExtended */
static int hf_denm_numberOfOccupants = -1;        /* NumberOfOccupants */
static int hf_denm_vehicleIdentification = -1;    /* VehicleIdentification */
static int hf_denm_energyStorageType = -1;        /* EnergyStorageType */
static int hf_denm_lanePosition = -1;             /* LanePosition */
static int hf_denm_impactReduction = -1;          /* ImpactReductionContainer */
static int hf_denm_externalTemperature = -1;      /* Temperature */
static int hf_denm_roadWorks = -1;                /* RoadWorksContainerExtended */
static int hf_denm_positioningSolution = -1;      /* PositioningSolutionType */
static int hf_denm_stationaryVehicle = -1;        /* StationaryVehicleContainer */
static int hf_denm_ReferenceDenms_item = -1;      /* ActionID */

/* --- Module TIS-TPG-Transactions-Descriptions --- --- ---                   */

static int hf_tistpg_tistpg_TisTpgTransaction_PDU = -1;  /* TisTpgTransaction */
static int hf_tistpg_drm = -1;                    /* TisTpgDRM */
static int hf_tistpg_snm = -1;                    /* TisTpgSNM */
static int hf_tistpg_trm = -1;                    /* TisTpgTRM */
static int hf_tistpg_tcm = -1;                    /* TisTpgTCM */
static int hf_tistpg_vdrm = -1;                   /* TisTpgVDRM */
static int hf_tistpg_vdpm = -1;                   /* TisTpgVDPM */
static int hf_tistpg_eofm = -1;                   /* TisTpgEOFM */
static int hf_tistpg_drmManagement = -1;          /* TisTpgDRM_Management */
static int hf_tistpg_drmSituation = -1;           /* TisTpgDRM_Situation */
static int hf_tistpg_drmLocation = -1;            /* TisTpgDRM_Location */
static int hf_tistpg_generationTime = -1;         /* TimestampIts */
static int hf_tistpg_vehicleType = -1;            /* UNVehicleClassifcation */
static int hf_tistpg_costumerContract = -1;       /* CustomerContract */
static int hf_tistpg_tisProfile = -1;             /* TisProfile */
static int hf_tistpg_causeCode = -1;              /* CauseCode */
static int hf_tistpg_vehiclePosition = -1;        /* ReferencePosition */
static int hf_tistpg_vehicleSpeed = -1;           /* Speed */
static int hf_tistpg_vehicleHeading = -1;         /* Heading */
static int hf_tistpg_requestedPosition = -1;      /* ReferencePosition */
static int hf_tistpg_searchRange = -1;            /* SearchRange */
static int hf_tistpg_searchCondition = -1;        /* SearchCondition */
static int hf_tistpg_snmManagement = -1;          /* TisTpgSNM_Management */
static int hf_tistpg_tpgContainer = -1;           /* TpgNotifContainer */
static int hf_tistpg_totalTpgStations = -1;       /* TotalTpgStations */
static int hf_tistpg_trmManagement = -1;          /* TisTpgTRM_Management */
static int hf_tistpg_trmSituation = -1;           /* TisTpgTRM_Situation */
static int hf_tistpg_trmLocation = -1;            /* TisTpgTRM_Location */
static int hf_tistpg_tpgStationID = -1;           /* StationID */
static int hf_tistpg_reservationStatus = -1;      /* ReservationStatus */
static int hf_tistpg_costumercontract = -1;       /* CustomerContract */
static int hf_tistpg_reservationID = -1;          /* ReservationID */
static int hf_tistpg_estArrivalTime = -1;         /* TimestampIts */
static int hf_tistpg_proposedPairingID = -1;      /* PairingID */
static int hf_tistpg_tcmManagement = -1;          /* TisTpgTCM_Management */
static int hf_tistpg_tcmSituation = -1;           /* TisTpgTCM_Situation */
static int hf_tistpg_tcmLocation = -1;            /* TisTpgTCM_Location */
static int hf_tistpg_reservedTpg = -1;            /* INTEGER_1_65535 */
static int hf_tistpg_tpgAutomationLevel = -1;     /* TpgAutomation */
static int hf_tistpg_pairingID = -1;              /* PairingID */
static int hf_tistpg_reservationTimeLimit = -1;   /* TimestampIts */
static int hf_tistpg_cancellationCondition = -1;  /* CancellationCondition */
static int hf_tistpg_tpgLocation = -1;            /* ReferencePosition */
static int hf_tistpg_address = -1;                /* UTF8String_SIZE_1_128 */
static int hf_tistpg_vdrmManagement = -1;         /* TisTpgVDRM_Management */
static int hf_tistpg_fillingStatus = -1;          /* FillingStatus */
static int hf_tistpg_automationLevel = -1;        /* TpgAutomation */
static int hf_tistpg_vdpmManagement = -1;         /* TisTpgVDPM_Management */
static int hf_tistpg_placardTable = -1;           /* PlacardTable */
static int hf_tistpg_vehicleSpecificData = -1;    /* VehicleSpecificData */
static int hf_tistpg_language = -1;               /* Language */
static int hf_tistpg_tyreTempCondition = -1;      /* TyreTempCondition */
static int hf_tistpg_currentVehicleConfiguration = -1;  /* PressureConfiguration */
static int hf_tistpg_frontLeftTyreData = -1;      /* TyreData */
static int hf_tistpg_frontRightTyreData = -1;     /* TyreData */
static int hf_tistpg_rearLeftTyreData = -1;       /* TyreData */
static int hf_tistpg_rearRightTyreData = -1;      /* TyreData */
static int hf_tistpg_spareTyreData = -1;          /* TyreData */
static int hf_tistpg_eofmManagement = -1;         /* TisTpgEOFM_Management */
static int hf_tistpg_numberOfAppliedPressure = -1;  /* NumberOfAppliedPressure */
static int hf_tistpg_appliedTyrePressures = -1;   /* AppliedTyrePressures */
static int hf_tistpg_PlacardTable_item = -1;      /* TyreSetVariant */
static int hf_tistpg_variantID = -1;              /* TyreSetVariantID */
static int hf_tistpg_frontAxleDimension = -1;     /* TyreSidewallInformation */
static int hf_tistpg_rearAxleDimension = -1;      /* TyreSidewallInformation */
static int hf_tistpg_pressureVariantsList = -1;   /* PressureVariantsList */
static int hf_tistpg_PressureVariantsList_item = -1;  /* PressureVariant */
static int hf_tistpg_pressureConfiguration = -1;  /* PressureConfiguration */
static int hf_tistpg_frontAxlePressure = -1;      /* AxlePlacardPressure */
static int hf_tistpg_rearAxlePressure = -1;       /* AxlePlacardPressure */
static int hf_tistpg_currentTyrePressure = -1;    /* T_currentTyrePressure */
static int hf_tistpg_tyrePressureValue = -1;      /* TyrePressure */
static int hf_tistpg_unavailable = -1;            /* NULL */
static int hf_tistpg_tyreSidewallInformation = -1;  /* T_tyreSidewallInformation */
static int hf_tistpg_tyreSidewallInformationValue = -1;  /* TyreSidewallInformation */
static int hf_tistpg_currentInsideAirTemperature = -1;  /* T_currentInsideAirTemperature */
static int hf_tistpg_tyreAirTemperatureValue = -1;  /* TyreAirTemperature */
static int hf_tistpg_recommendedTyrePressure = -1;  /* T_recommendedTyrePressure */
static int hf_tistpg_axlePlacardPressureValue = -1;  /* AxlePlacardPressure */
static int hf_tistpg_tin = -1;                    /* T_tin */
static int hf_tistpg_tinValue = -1;               /* TIN */
static int hf_tistpg_sensorState = -1;            /* T_sensorState */
static int hf_tistpg_sensorStateValue = -1;       /* SensorState */
static int hf_tistpg_tpgNumber = -1;              /* TpgNumber */
static int hf_tistpg_tpgProvider = -1;            /* TpgProvider */
static int hf_tistpg_accessibility = -1;          /* Accessibility */
static int hf_tistpg_phoneNumber = -1;            /* PhoneNumber */
static int hf_tistpg_digitalMap = -1;             /* DigitalMap */
static int hf_tistpg_openingDaysHours = -1;       /* OpeningDaysHours */
static int hf_tistpg_bookingInfo = -1;            /* BookingInfo */
static int hf_tistpg_availableTpgNumber = -1;     /* AvailableTpgNumber */
static int hf_tistpg_AppliedTyrePressures_item = -1;  /* AppliedTyrePressure */
static int hf_tistpg_TpgNotifContainer_item = -1;  /* TpgStationData */
/* named bits */
static int hf_tistpg_TpgAutomation_fullAutomated = -1;
static int hf_tistpg_TpgAutomation_semiAutomated = -1;
static int hf_tistpg_TpgAutomation_manual = -1;
static int hf_tistpg_TpgAutomation_reserved = -1;
static int hf_tistpg_TisProfile_reserved = -1;
static int hf_tistpg_TisProfile_profileOne = -1;
static int hf_tistpg_TisProfile_profileTwo = -1;
static int hf_tistpg_TisProfile_profileThree = -1;

/* --- Module EVCSN-PDU-Descriptions --- --- ---                              */

static int hf_evcsn_evcsn_EVChargingSpotNotificationPOIMessage_PDU = -1;  /* EVChargingSpotNotificationPOIMessage */
static int hf_evcsn_poiHeader = -1;               /* ItsPOIHeader */
static int hf_evcsn_evcsnData = -1;               /* ItsEVCSNData */
static int hf_evcsn_poiType = -1;                 /* POIType */
static int hf_evcsn_timeStamp = -1;               /* TimestampIts */
static int hf_evcsn_relayCapable = -1;            /* BOOLEAN */
static int hf_evcsn_totalNumberOfStations = -1;   /* NumberStations */
static int hf_evcsn_chargingStationsData = -1;    /* SEQUENCE_SIZE_1_256_OF_ItsChargingStationData */
static int hf_evcsn_chargingStationsData_item = -1;  /* ItsChargingStationData */
static int hf_evcsn_chargingStationID = -1;       /* StationID */
static int hf_evcsn_utilityDistributorId = -1;    /* UTF8String_SIZE_1_32 */
static int hf_evcsn_providerID = -1;              /* UTF8String_SIZE_1_32 */
static int hf_evcsn_chargingStationLocation = -1;  /* ReferencePosition */
static int hf_evcsn_address = -1;                 /* UTF8String */
static int hf_evcsn_phoneNumber = -1;             /* NumericString_SIZE_1_16 */
static int hf_evcsn_accessibility = -1;           /* UTF8String_SIZE_1_32 */
static int hf_evcsn_digitalMap = -1;              /* DigitalMap */
static int hf_evcsn_openingDaysHours = -1;        /* UTF8String */
static int hf_evcsn_pricing = -1;                 /* UTF8String */
static int hf_evcsn_bookingContactInfo = -1;      /* UTF8String */
static int hf_evcsn_payment = -1;                 /* UTF8String */
static int hf_evcsn_chargingSpotsAvailable = -1;  /* ItsChargingSpots */
static int hf_evcsn_ItsChargingSpots_item = -1;   /* ItsChargingSpotDataElements */
static int hf_evcsn_type = -1;                    /* ChargingSpotType */
static int hf_evcsn_evEquipmentID = -1;           /* UTF8String */
static int hf_evcsn_typeOfReceptacle = -1;        /* TypeOfReceptacle */
static int hf_evcsn_energyAvailability = -1;      /* UTF8String */
static int hf_evcsn_parkingPlacesData = -1;       /* ParkingPlacesData */
static int hf_evcsn_ParkingPlacesData_item = -1;  /* SpotAvailability */
static int hf_evcsn_maxWaitingTimeMinutes = -1;   /* INTEGER_0_1400 */
static int hf_evcsn_blocking = -1;                /* BOOLEAN */
/* named bits */
static int hf_evcsn_ChargingSpotType_standardChargeMode1 = -1;
static int hf_evcsn_ChargingSpotType_standardChargeMode2 = -1;
static int hf_evcsn_ChargingSpotType_standardOrFastChargeMode3 = -1;
static int hf_evcsn_ChargingSpotType_fastChargeWithExternalCharger = -1;
static int hf_evcsn_ChargingSpotType_quickDrop = -1;
static int hf_evcsn_ChargingSpotType_inductiveChargeWhileStationary = -1;
static int hf_evcsn_ChargingSpotType_inductiveChargeWhileDriving = -1;

/* --- Module EV-RechargingSpotReservation-PDU-Descriptions --- --- ---       */

static int hf_evrsr_evrsr_EV_RSR_MessageBody_PDU = -1;  /* EV_RSR_MessageBody */
static int hf_evrsr_preReservationRequestMessage = -1;  /* PreReservationRequestMessage */
static int hf_evrsr_preReservationResponseMessage = -1;  /* PreReservationResponseMessage */
static int hf_evrsr_reservationRequestMessage = -1;  /* ReservationRequestMessage */
static int hf_evrsr_reservationResponseMessage = -1;  /* ReservationResponseMessage */
static int hf_evrsr_cancellationRequestMessage = -1;  /* CancellationRequestMessage */
static int hf_evrsr_cancellationResponseMessage = -1;  /* CancellationResponseMessage */
static int hf_evrsr_updateRequestMessage = -1;    /* UpdateRequestMessage */
static int hf_evrsr_updateResponseMessage = -1;   /* UpdateResponseMessage */
static int hf_evrsr_evse_ID = -1;                 /* EVSE_ID */
static int hf_evrsr_arrivalTime = -1;             /* TimestampUTC */
static int hf_evrsr_departureTime = -1;           /* TimestampUTC */
static int hf_evrsr_rechargingType = -1;          /* RechargingType */
static int hf_evrsr_batteryType = -1;             /* BatteryType */
static int hf_evrsr_preReservation_ID = -1;       /* PreReservation_ID */
static int hf_evrsr_availabilityStatus = -1;      /* AvailabilityStatus */
static int hf_evrsr_preReservationExpirationTime = -1;  /* TimestampUTC */
static int hf_evrsr_supportedPaymentTypes = -1;   /* SupportedPaymentTypes */
static int hf_evrsr_currentTime = -1;             /* TimestampUTC */
static int hf_evrsr_eAmount = -1;                 /* EAmount */
static int hf_evrsr_eAmountMin = -1;              /* EAmount */
static int hf_evrsr_paymentType = -1;             /* PaymentType */
static int hf_evrsr_payment_ID = -1;              /* Payment_ID */
static int hf_evrsr_secondPayment_ID = -1;        /* Payment_ID */
static int hf_evrsr_pairing_ID = -1;              /* Pairing_ID */
static int hf_evrsr_reservationResponseCode = -1;  /* ReservationResponseCode */
static int hf_evrsr_reservation_ID = -1;          /* Reservation_ID */
static int hf_evrsr_reservation_Password = -1;    /* Reservation_Password */
static int hf_evrsr_stationDetails = -1;          /* StationDetails */
static int hf_evrsr_chargingSpotLabel = -1;       /* ChargingSpotLabel */
static int hf_evrsr_expirationTime = -1;          /* TimestampUTC */
static int hf_evrsr_freeCancelTimeLimit = -1;     /* TimestampUTC */
static int hf_evrsr_cancellationResponseCode = -1;  /* CancellationResponseCode */
static int hf_evrsr_updatedArrivalTime = -1;      /* TimestampUTC */
static int hf_evrsr_updatedDepartureTime = -1;    /* TimestampUTC */
static int hf_evrsr_updateResponseCode = -1;      /* UpdateResponseCode */
static int hf_evrsr_contractID = -1;              /* ContractID */
static int hf_evrsr_externalIdentificationMeans = -1;  /* ExternalIdentificationMeans */
static int hf_evrsr_rechargingMode = -1;          /* RechargingMode */
static int hf_evrsr_powerSource = -1;             /* PowerSource */
/* named bits */
static int hf_evrsr_SupportedPaymentTypes_contract = -1;
static int hf_evrsr_SupportedPaymentTypes_externalIdentification = -1;

/*--- End of included file: packet-its-hf.c ---*/
#line 271 "./asn1/its/packet-its-template.c"

// CauseCode/SubCauseCode management
static int hf_its_trafficConditionSubCauseCode = -1;
static int hf_its_accidentSubCauseCode = -1;
static int hf_its_roadworksSubCauseCode = -1;
static int hf_its_adverseWeatherCondition_PrecipitationSubCauseCode = -1;
static int hf_its_adverseWeatherCondition_VisibilitySubCauseCode = -1;
static int hf_its_adverseWeatherCondition_AdhesionSubCauseCode = -1;
static int hf_its_adverseWeatherCondition_ExtremeWeatherConditionSubCauseCode = -1;
static int hf_its_hazardousLocation_AnimalOnTheRoadSubCauseCode = -1;
static int hf_its_hazardousLocation_ObstacleOnTheRoadSubCauseCode = -1;
static int hf_its_hazardousLocation_SurfaceConditionSubCauseCode = -1;
static int hf_its_hazardousLocation_DangerousCurveSubCauseCode = -1;
static int hf_its_humanPresenceOnTheRoadSubCauseCode = -1;
static int hf_its_wrongWayDrivingSubCauseCode = -1;
static int hf_its_rescueAndRecoveryWorkInProgressSubCauseCode = -1;
static int hf_its_slowVehicleSubCauseCode = -1;
static int hf_its_dangerousEndOfQueueSubCauseCode = -1;
static int hf_its_vehicleBreakdownSubCauseCode = -1;
static int hf_its_postCrashSubCauseCode = -1;
static int hf_its_humanProblemSubCauseCode = -1;
static int hf_its_stationaryVehicleSubCauseCode = -1;
static int hf_its_emergencyVehicleApproachingSubCauseCode = -1;
static int hf_its_collisionRiskSubCauseCode = -1;
static int hf_its_signalViolationSubCauseCode = -1;
static int hf_its_dangerousSituationSubCauseCode = -1;

static gint ett_its = -1;


/*--- Included file: packet-its-ett.c ---*/
#line 1 "./asn1/its/packet-its-ett.c"

/* --- Module ITS-Container --- --- ---                                       */

static gint ett_its_ItsPduHeader = -1;
static gint ett_its_ReferencePosition = -1;
static gint ett_its_DeltaReferencePosition = -1;
static gint ett_its_Altitude = -1;
static gint ett_its_PosConfidenceEllipse = -1;
static gint ett_its_PathPoint = -1;
static gint ett_its_PtActivation = -1;
static gint ett_its_AccelerationControl = -1;
static gint ett_its_CauseCode = -1;
static gint ett_its_Curvature = -1;
static gint ett_its_Heading = -1;
static gint ett_its_ClosedLanes = -1;
static gint ett_its_Speed = -1;
static gint ett_its_LongitudinalAcceleration = -1;
static gint ett_its_LateralAcceleration = -1;
static gint ett_its_VerticalAcceleration = -1;
static gint ett_its_ExteriorLights = -1;
static gint ett_its_DangerousGoodsExtended = -1;
static gint ett_its_SpecialTransportType = -1;
static gint ett_its_LightBarSirenInUse = -1;
static gint ett_its_PositionOfOccupants = -1;
static gint ett_its_VehicleIdentification = -1;
static gint ett_its_EnergyStorageType = -1;
static gint ett_its_VehicleLength = -1;
static gint ett_its_PathHistory = -1;
static gint ett_its_EmergencyPriority = -1;
static gint ett_its_SteeringWheelAngle = -1;
static gint ett_its_YawRate = -1;
static gint ett_its_ActionID = -1;
static gint ett_its_ItineraryPath = -1;
static gint ett_its_ProtectedCommunicationZone = -1;
static gint ett_its_Traces = -1;
static gint ett_its_PositionOfPillars = -1;
static gint ett_its_RestrictedTypes = -1;
static gint ett_its_EventHistory = -1;
static gint ett_its_EventPoint = -1;
static gint ett_its_ProtectedCommunicationZonesRSU = -1;
static gint ett_its_CenDsrcTollingZone = -1;
static gint ett_its_DigitalMap = -1;

/* --- Module AVIAEINumberingAndDataStructures --- --- ---                    */


/* --- Module ElectronicRegistrationIdentificationVehicleDataModule --- --- --- */

static gint ett_erivdm_EuVehicleCategoryCode = -1;

/* --- Module CITSapplMgmtIDs --- --- ---                                     */

static gint ett_csmid_VarLengthNumber = -1;
static gint ett_csmid_Ext1 = -1;
static gint ett_csmid_Ext2 = -1;

/* --- Module EfcDsrcApplication --- --- ---                                  */

static gint ett_dsrc_app_AxleWeightLimits = -1;
static gint ett_dsrc_app_DieselEmissionValues = -1;
static gint ett_dsrc_app_T_particulate = -1;
static gint ett_dsrc_app_EnvironmentalCharacteristics = -1;
static gint ett_dsrc_app_ExhaustEmissionValues = -1;
static gint ett_dsrc_app_PassengerCapacity = -1;
static gint ett_dsrc_app_Provider = -1;
static gint ett_dsrc_app_SoundLevel = -1;
static gint ett_dsrc_app_VehicleDimensions = -1;
static gint ett_dsrc_app_VehicleWeightLimits = -1;

/* --- Module IVI --- --- ---                                                 */

static gint ett_ivi_IviStructure = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_8__OF_IviContainer = -1;
static gint ett_ivi_IviContainer = -1;
static gint ett_ivi_IVIManagementContainer = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_8_OF_IviIdentificationNumber = -1;
static gint ett_ivi_GeographicLocationContainer = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_16__OF_GlcPart = -1;
static gint ett_ivi_GlcPart = -1;
static gint ett_ivi_GeneralIviContainer = -1;
static gint ett_ivi_GicPart = -1;
static gint ett_ivi_T_GicPartDetectionZoneIds = -1;
static gint ett_ivi_T_GicPartRelevanceZoneIds = -1;
static gint ett_ivi_T_GicPartDriverAwarenessZoneIds = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_8__OF_LanePosition = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_8__OF_CompleteVehicleCharacteristics = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_4__OF_RSCode = -1;
static gint ett_ivi_T_GicPartExtraText = -1;
static gint ett_ivi_RoadConfigurationContainer = -1;
static gint ett_ivi_RccPart = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_8__OF_Zid = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_16__OF_LaneInformation = -1;
static gint ett_ivi_TextContainer = -1;
static gint ett_ivi_TcPart = -1;
static gint ett_ivi_T_TcPartDetectionZoneIds = -1;
static gint ett_ivi_T_TcPartRelevanceZoneIds = -1;
static gint ett_ivi_T_TcPartDriverAwarenessZoneIds = -1;
static gint ett_ivi_T_TcPartText = -1;
static gint ett_ivi_LayoutContainer = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_4__OF_LayoutComponent = -1;
static gint ett_ivi_AbsolutePosition = -1;
static gint ett_ivi_AbsolutePositionWAltitude = -1;
static gint ett_ivi_AnyCatalogue = -1;
static gint ett_ivi_CompleteVehicleCharacteristics = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_3_OF_TrailerCharacteristics = -1;
static gint ett_ivi_ComputedSegment = -1;
static gint ett_ivi_DeltaPosition = -1;
static gint ett_ivi_Distance = -1;
static gint ett_ivi_DistanceOrDuration = -1;
static gint ett_ivi_ISO14823Attributes = -1;
static gint ett_ivi_ISO14823Attributes_item = -1;
static gint ett_ivi_ISO14823Code = -1;
static gint ett_ivi_T_icPictogramCode = -1;
static gint ett_ivi_T_serviceCategoryCode = -1;
static gint ett_ivi_T_pictogramCategoryCode = -1;
static gint ett_ivi_LaneInformation = -1;
static gint ett_ivi_LayoutComponent = -1;
static gint ett_ivi_LoadType = -1;
static gint ett_ivi_PolygonalLine = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_32__OF_DeltaPosition = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_32__OF_DeltaReferencePosition = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePosition = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePositionWAltitude = -1;
static gint ett_ivi_RSCode = -1;
static gint ett_ivi_T_code = -1;
static gint ett_ivi_Segment = -1;
static gint ett_ivi_Text = -1;
static gint ett_ivi_TractorCharacteristics = -1;
static gint ett_ivi_T_TractorCharactEqualTo = -1;
static gint ett_ivi_T_TractorCharactNotEqualTo = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_4__OF_VehicleCharacteristicsRanges = -1;
static gint ett_ivi_TrailerCharacteristics = -1;
static gint ett_ivi_T_TrailerCharactEqualTo = -1;
static gint ett_ivi_T_TrailerCharactNotEqualTo = -1;
static gint ett_ivi_VcCode = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_8__OF_DTM = -1;
static gint ett_ivi_VehicleCharacteristicsFixValues = -1;
static gint ett_ivi_VehicleCharacteristicsRanges = -1;
static gint ett_ivi_T_limits = -1;
static gint ett_ivi_Weight = -1;
static gint ett_ivi_Zone = -1;
static gint ett_ivi_DTM = -1;
static gint ett_ivi_T_year = -1;
static gint ett_ivi_T_month_day = -1;
static gint ett_ivi_T_hourMinutes = -1;
static gint ett_ivi_MonthDay = -1;
static gint ett_ivi_PMD = -1;
static gint ett_ivi_HoursMinutes = -1;
static gint ett_ivi_DayOfWeek = -1;
static gint ett_ivi_VED = -1;
static gint ett_ivi_SPE = -1;
static gint ett_ivi_DDD = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_8__OF_DDD_IO = -1;
static gint ett_ivi_DDD_IO = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_4__OF_DestinationPlace = -1;
static gint ett_ivi_SEQUENCE_SIZE_1_4__OF_DestinationRoad = -1;
static gint ett_ivi_DestinationPlace = -1;
static gint ett_ivi_DestinationRoad = -1;

/* --- Module DSRC --- --- ---                                                */

static gint ett_dsrc_RegionalExtension = -1;
static gint ett_dsrc_SPAT = -1;
static gint ett_dsrc_T_SPATRegional = -1;
static gint ett_dsrc_SignalRequestMessage = -1;
static gint ett_dsrc_T_SRMRegional = -1;
static gint ett_dsrc_SignalStatusMessage = -1;
static gint ett_dsrc_T_SSMRegional = -1;
static gint ett_dsrc_MapData = -1;
static gint ett_dsrc_T_MAPRegional = -1;
static gint ett_dsrc_AdvisorySpeed = -1;
static gint ett_dsrc_T_AdvisorySpeedRegional = -1;
static gint ett_dsrc_AdvisorySpeedList = -1;
static gint ett_dsrc_ComputedLane = -1;
static gint ett_dsrc_T_offsetXaxis = -1;
static gint ett_dsrc_T_offsetYaxis = -1;
static gint ett_dsrc_T_ComputedLaneRegional = -1;
static gint ett_dsrc_ConnectsToList = -1;
static gint ett_dsrc_ConnectingLane = -1;
static gint ett_dsrc_Connection = -1;
static gint ett_dsrc_ConnectionManeuverAssist = -1;
static gint ett_dsrc_T_ConnectionManeuverAssistRegional = -1;
static gint ett_dsrc_DataParameters = -1;
static gint ett_dsrc_EnabledLaneList = -1;
static gint ett_dsrc_GenericLane = -1;
static gint ett_dsrc_T_GenericLaneRegional = -1;
static gint ett_dsrc_IntersectionAccessPoint = -1;
static gint ett_dsrc_IntersectionGeometry = -1;
static gint ett_dsrc_T_IntersectionGeometryRegional = -1;
static gint ett_dsrc_IntersectionGeometryList = -1;
static gint ett_dsrc_IntersectionReferenceID = -1;
static gint ett_dsrc_IntersectionState = -1;
static gint ett_dsrc_T_IntersectionStateRegional = -1;
static gint ett_dsrc_IntersectionStateList = -1;
static gint ett_dsrc_LaneAttributes = -1;
static gint ett_dsrc_LaneDataAttribute = -1;
static gint ett_dsrc_T_LaneDataAttributeRegional = -1;
static gint ett_dsrc_LaneDataAttributeList = -1;
static gint ett_dsrc_LaneList = -1;
static gint ett_dsrc_LaneSharing = -1;
static gint ett_dsrc_LaneTypeAttributes = -1;
static gint ett_dsrc_ManeuverAssistList = -1;
static gint ett_dsrc_MovementEventList = -1;
static gint ett_dsrc_MovementEvent = -1;
static gint ett_dsrc_T_MovementEventRegional = -1;
static gint ett_dsrc_MovementList = -1;
static gint ett_dsrc_MovementState = -1;
static gint ett_dsrc_T_MovementStateRegional = -1;
static gint ett_dsrc_NodeAttributeSetXY = -1;
static gint ett_dsrc_T_NodeAttributeSetXYRegional = -1;
static gint ett_dsrc_NodeAttributeXYList = -1;
static gint ett_dsrc_Node_LLmD_64b = -1;
static gint ett_dsrc_Node_XY_20b = -1;
static gint ett_dsrc_Node_XY_22b = -1;
static gint ett_dsrc_Node_XY_24b = -1;
static gint ett_dsrc_Node_XY_26b = -1;
static gint ett_dsrc_Node_XY_28b = -1;
static gint ett_dsrc_Node_XY_32b = -1;
static gint ett_dsrc_NodeListXY = -1;
static gint ett_dsrc_NodeOffsetPointXY = -1;
static gint ett_dsrc_NodeSetXY = -1;
static gint ett_dsrc_NodeXY = -1;
static gint ett_dsrc_OverlayLaneList = -1;
static gint ett_dsrc_Position3D = -1;
static gint ett_dsrc_T_Position3DRegional = -1;
static gint ett_dsrc_PreemptPriorityList = -1;
static gint ett_dsrc_RegulatorySpeedLimit = -1;
static gint ett_dsrc_RequestorDescription = -1;
static gint ett_dsrc_T_RequestorDescriptionRegional = -1;
static gint ett_dsrc_RequestorPositionVector = -1;
static gint ett_dsrc_RequestorType = -1;
static gint ett_dsrc_RestrictionClassAssignment = -1;
static gint ett_dsrc_RestrictionClassList = -1;
static gint ett_dsrc_RestrictionUserTypeList = -1;
static gint ett_dsrc_RestrictionUserType = -1;
static gint ett_dsrc_T_RestrictionUserTypeRegional = -1;
static gint ett_dsrc_RoadLaneSetList = -1;
static gint ett_dsrc_RoadSegmentReferenceID = -1;
static gint ett_dsrc_RoadSegment = -1;
static gint ett_dsrc_T_RoadSegmentRegional = -1;
static gint ett_dsrc_RoadSegmentList = -1;
static gint ett_dsrc_SegmentAttributeXYList = -1;
static gint ett_dsrc_SignalControlZone = -1;
static gint ett_dsrc_SignalRequesterInfo = -1;
static gint ett_dsrc_SignalRequestList = -1;
static gint ett_dsrc_SignalRequestPackage = -1;
static gint ett_dsrc_T_SignalRequestPackageRegional = -1;
static gint ett_dsrc_SignalRequest = -1;
static gint ett_dsrc_T_SignalRequestRegional = -1;
static gint ett_dsrc_SignalStatusList = -1;
static gint ett_dsrc_SignalStatusPackageList = -1;
static gint ett_dsrc_SignalStatusPackage = -1;
static gint ett_dsrc_T_SignalStatusPackageRegional = -1;
static gint ett_dsrc_SignalStatus = -1;
static gint ett_dsrc_T_SignalStatusRegional = -1;
static gint ett_dsrc_SpeedLimitList = -1;
static gint ett_dsrc_TimeChangeDetails = -1;
static gint ett_dsrc_TransmissionAndSpeed = -1;
static gint ett_dsrc_VehicleID = -1;
static gint ett_dsrc_AllowedManeuvers = -1;
static gint ett_dsrc_IntersectionStatusObject = -1;
static gint ett_dsrc_LaneAttributes_Barrier = -1;
static gint ett_dsrc_LaneAttributes_Bike = -1;
static gint ett_dsrc_LaneAttributes_Crosswalk = -1;
static gint ett_dsrc_LaneAttributes_Parking = -1;
static gint ett_dsrc_LaneAttributes_Sidewalk = -1;
static gint ett_dsrc_LaneAttributes_Striping = -1;
static gint ett_dsrc_LaneAttributes_TrackedVehicle = -1;
static gint ett_dsrc_LaneAttributes_Vehicle = -1;
static gint ett_dsrc_LaneDirection = -1;
static gint ett_dsrc_TransitVehicleStatus = -1;

/* --- Module AddGrpC --- --- ---                                             */

static gint ett_AddGrpC_ConnectionManeuverAssist_addGrpC = -1;
static gint ett_AddGrpC_ConnectionTrajectory_addGrpC = -1;
static gint ett_AddGrpC_Control_addGrpC = -1;
static gint ett_AddGrpC_IntersectionState_addGrpC = -1;
static gint ett_AddGrpC_MapData_addGrpC = -1;
static gint ett_AddGrpC_Position3D_addGrpC = -1;
static gint ett_AddGrpC_PrioritizationResponseList = -1;
static gint ett_AddGrpC_PrioritizationResponse = -1;
static gint ett_AddGrpC_RestrictionUserType_addGrpC = -1;
static gint ett_AddGrpC_SignalHeadLocationList = -1;
static gint ett_AddGrpC_SignalHeadLocation = -1;
static gint ett_AddGrpC_SignalStatusPackage_addGrpC = -1;
static gint ett_AddGrpC_ItsStationPositionList = -1;
static gint ett_AddGrpC_ItsStationPosition = -1;

/* --- Module REGION --- --- ---                                              */


/* --- Module SPATEM-PDU-Descriptions --- --- ---                             */


/* --- Module MAPEM-PDU-Descriptions --- --- ---                              */


/* --- Module IVIM-PDU-Descriptions --- --- ---                               */


/* --- Module SREM-PDU-Descriptions --- --- ---                               */


/* --- Module SSEM-PDU-Descriptions --- --- ---                               */


/* --- Module CAM-PDU-Descriptions --- --- ---                                */

static gint ett_cam_CoopAwareness = -1;
static gint ett_cam_CamParameters = -1;
static gint ett_cam_HighFrequencyContainer = -1;
static gint ett_cam_LowFrequencyContainer = -1;
static gint ett_cam_SpecialVehicleContainer = -1;
static gint ett_cam_BasicContainer = -1;
static gint ett_cam_BasicVehicleContainerHighFrequency = -1;
static gint ett_cam_BasicVehicleContainerLowFrequency = -1;
static gint ett_cam_PublicTransportContainer = -1;
static gint ett_cam_SpecialTransportContainer = -1;
static gint ett_cam_DangerousGoodsContainer = -1;
static gint ett_cam_RoadWorksContainerBasic = -1;
static gint ett_cam_RescueContainer = -1;
static gint ett_cam_EmergencyContainer = -1;
static gint ett_cam_SafetyCarContainer = -1;
static gint ett_cam_RSUContainerHighFrequency = -1;

/* --- Module DENM-PDU-Descriptions --- --- ---                               */

static gint ett_denm_DecentralizedEnvironmentalNotificationMessage = -1;
static gint ett_denm_ManagementContainer = -1;
static gint ett_denm_SituationContainer = -1;
static gint ett_denm_LocationContainer = -1;
static gint ett_denm_ImpactReductionContainer = -1;
static gint ett_denm_RoadWorksContainerExtended = -1;
static gint ett_denm_StationaryVehicleContainer = -1;
static gint ett_denm_AlacarteContainer = -1;
static gint ett_denm_ReferenceDenms = -1;

/* --- Module TIS-TPG-Transactions-Descriptions --- --- ---                   */

static gint ett_tistpg_TisTpgTransaction = -1;
static gint ett_tistpg_TisTpgDRM = -1;
static gint ett_tistpg_TisTpgDRM_Management = -1;
static gint ett_tistpg_TisTpgDRM_Situation = -1;
static gint ett_tistpg_TisTpgDRM_Location = -1;
static gint ett_tistpg_TisTpgSNM = -1;
static gint ett_tistpg_TisTpgSNM_Management = -1;
static gint ett_tistpg_TisTpgTRM = -1;
static gint ett_tistpg_TisTpgTRM_Management = -1;
static gint ett_tistpg_TisTpgTRM_Situation = -1;
static gint ett_tistpg_TisTpgTRM_Location = -1;
static gint ett_tistpg_TisTpgTCM = -1;
static gint ett_tistpg_TisTpgTCM_Management = -1;
static gint ett_tistpg_TisTpgTCM_Situation = -1;
static gint ett_tistpg_TisTpgTCM_Location = -1;
static gint ett_tistpg_TisTpgVDRM = -1;
static gint ett_tistpg_TisTpgVDRM_Management = -1;
static gint ett_tistpg_TisTpgVDPM = -1;
static gint ett_tistpg_TisTpgVDPM_Management = -1;
static gint ett_tistpg_VehicleSpecificData = -1;
static gint ett_tistpg_TisTpgEOFM = -1;
static gint ett_tistpg_TisTpgEOFM_Management = -1;
static gint ett_tistpg_PlacardTable = -1;
static gint ett_tistpg_TyreSetVariant = -1;
static gint ett_tistpg_PressureVariantsList = -1;
static gint ett_tistpg_PressureVariant = -1;
static gint ett_tistpg_TyreData = -1;
static gint ett_tistpg_T_currentTyrePressure = -1;
static gint ett_tistpg_T_tyreSidewallInformation = -1;
static gint ett_tistpg_T_currentInsideAirTemperature = -1;
static gint ett_tistpg_T_recommendedTyrePressure = -1;
static gint ett_tistpg_T_tin = -1;
static gint ett_tistpg_T_sensorState = -1;
static gint ett_tistpg_AppliedTyrePressure = -1;
static gint ett_tistpg_TpgStationData = -1;
static gint ett_tistpg_AppliedTyrePressures = -1;
static gint ett_tistpg_TpgNotifContainer = -1;
static gint ett_tistpg_TpgAutomation = -1;
static gint ett_tistpg_TisProfile = -1;

/* --- Module EVCSN-PDU-Descriptions --- --- ---                              */

static gint ett_evcsn_EVChargingSpotNotificationPOIMessage = -1;
static gint ett_evcsn_ItsPOIHeader = -1;
static gint ett_evcsn_ItsEVCSNData = -1;
static gint ett_evcsn_SEQUENCE_SIZE_1_256_OF_ItsChargingStationData = -1;
static gint ett_evcsn_ItsChargingStationData = -1;
static gint ett_evcsn_ItsChargingSpots = -1;
static gint ett_evcsn_ItsChargingSpotDataElements = -1;
static gint ett_evcsn_ChargingSpotType = -1;
static gint ett_evcsn_ParkingPlacesData = -1;
static gint ett_evcsn_SpotAvailability = -1;

/* --- Module EV-RechargingSpotReservation-PDU-Descriptions --- --- ---       */

static gint ett_evrsr_EV_RSR_MessageBody = -1;
static gint ett_evrsr_PreReservationRequestMessage = -1;
static gint ett_evrsr_PreReservationResponseMessage = -1;
static gint ett_evrsr_ReservationRequestMessage = -1;
static gint ett_evrsr_ReservationResponseMessage = -1;
static gint ett_evrsr_CancellationRequestMessage = -1;
static gint ett_evrsr_CancellationResponseMessage = -1;
static gint ett_evrsr_UpdateRequestMessage = -1;
static gint ett_evrsr_UpdateResponseMessage = -1;
static gint ett_evrsr_Payment_ID = -1;
static gint ett_evrsr_RechargingType = -1;
static gint ett_evrsr_SupportedPaymentTypes = -1;

/*--- End of included file: packet-its-ett.c ---*/
#line 301 "./asn1/its/packet-its-template.c"

// Deal with cause/subcause code management
struct { CauseCodeType_enum cause; int* hf; } cause_to_subcause[] = {
    { trafficCondition, &hf_its_trafficConditionSubCauseCode },
    { accident, &hf_its_accidentSubCauseCode },
    { roadworks, &hf_its_roadworksSubCauseCode },
    { adverseWeatherCondition_Precipitation, &hf_its_adverseWeatherCondition_PrecipitationSubCauseCode },
    { adverseWeatherCondition_Visibility, &hf_its_adverseWeatherCondition_VisibilitySubCauseCode },
    { adverseWeatherCondition_Adhesion, &hf_its_adverseWeatherCondition_AdhesionSubCauseCode },
    { adverseWeatherCondition_ExtremeWeatherCondition, &hf_its_adverseWeatherCondition_ExtremeWeatherConditionSubCauseCode },
    { hazardousLocation_AnimalOnTheRoad, &hf_its_hazardousLocation_AnimalOnTheRoadSubCauseCode },
    { hazardousLocation_ObstacleOnTheRoad, &hf_its_hazardousLocation_ObstacleOnTheRoadSubCauseCode },
    { hazardousLocation_SurfaceCondition, &hf_its_hazardousLocation_SurfaceConditionSubCauseCode },
    { hazardousLocation_DangerousCurve, &hf_its_hazardousLocation_DangerousCurveSubCauseCode },
    { humanPresenceOnTheRoad, &hf_its_humanPresenceOnTheRoadSubCauseCode },
    { wrongWayDriving, &hf_its_wrongWayDrivingSubCauseCode },
    { rescueAndRecoveryWorkInProgress, &hf_its_rescueAndRecoveryWorkInProgressSubCauseCode },
    { slowVehicle, &hf_its_slowVehicleSubCauseCode },
    { dangerousEndOfQueue, &hf_its_dangerousEndOfQueueSubCauseCode },
    { vehicleBreakdown, &hf_its_vehicleBreakdownSubCauseCode },
    { postCrash, &hf_its_postCrashSubCauseCode },
    { humanProblem, &hf_its_humanProblemSubCauseCode },
    { stationaryVehicle, &hf_its_stationaryVehicleSubCauseCode },
    { emergencyVehicleApproaching, &hf_its_emergencyVehicleApproachingSubCauseCode },
    { collisionRisk, &hf_its_collisionRiskSubCauseCode },
    { signalViolation, &hf_its_signalViolationSubCauseCode },
    { dangerousSituation, &hf_its_dangerousSituationSubCauseCode },
    { reserved, NULL },
};

static int*
find_subcause_from_cause(CauseCodeType_enum cause)
{
    int idx = 0;

    while (cause_to_subcause[idx].hf && (cause_to_subcause[idx].cause != cause))
        idx++;

    return cause_to_subcause[idx].hf?cause_to_subcause[idx].hf:&hf_its_subCauseCode;
}


/*--- Included file: packet-its-fn.c ---*/
#line 1 "./asn1/its/packet-its-fn.c"

/* --- Module ITS-Container --- --- ---                                       */



static int
dissect_its_T_protocolVersion(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, &((its_header_t*)actx->private_data)->version, FALSE);

  return offset;
}


static const value_string its_T_messageID_vals[] = {
  { ITS_DENM, "denm" },
  { ITS_CAM, "cam" },
  { ITS_POI, "poi" },
  { ITS_SPATEM, "spatem" },
  { ITS_MAPEM, "mapem" },
  { ITS_IVIM, "ivim" },
  { ITS_EV_RSR, "ev-rsr" },
  { ITS_TISTPGTRANSACTION, "tistpgtransaction" },
  { ITS_SREM, "srem" },
  { ITS_SSEM, "ssem" },
  { ITS_EVCSN, "evcsn" },
  { ITS_SAEM, "saem" },
  { ITS_RTCMEM, "rtcmem" },
  { 0, NULL }
};


static int
dissect_its_T_messageID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, &((its_header_t*)actx->private_data)->msgId, FALSE);

  return offset;
}



static int
dissect_its_StationID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 4294967295U, actx->private_data?&((its_header_t*)actx->private_data)->stationId:NULL, FALSE);

  return offset;
}


static const per_sequence_t its_ItsPduHeader_sequence[] = {
  { &hf_its_protocolVersion , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_T_protocolVersion },
  { &hf_its_messageID       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_T_messageID },
  { &hf_its_stationID       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_StationID },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_ItsPduHeader(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 584 "./asn1/its/its.cnf"
  guint8 version = tvb_get_guint8(tvb, 0);
  if ((offset = dissector_try_uint(its_version_subdissector_table, version, tvb, actx->pinfo, tree))) {
    return offset;
  }
  // Lets try it that way, regarless of version value...
  its_header_t *hdr = wmem_new0(wmem_packet_scope(), its_header_t);
  actx->private_data = (void*)hdr;
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_ItsPduHeader, its_ItsPduHeader_sequence);

  tap_queue_packet(its_tap, actx->pinfo, actx->private_data);
  tvbuff_t *next_tvb = tvb_new_subset_length(tvb, (offset+7)>>3, -1);
  if (!dissector_try_uint(its_msgid_subdissector_table, (hdr->version << 16)+hdr->msgId, next_tvb, actx->pinfo, tree)) {
    call_data_dissector(next_tvb, actx->pinfo, tree);
  }


  return offset;
}


static const value_string its_Latitude_vals[] = {
  {  10, "oneMicrodegreeNorth" },
  { -10, "oneMicrodegreeSouth" },
  { 900000001, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_Latitude(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -900000000, 900000001U, NULL, FALSE);

  return offset;
}


static const value_string its_Longitude_vals[] = {
  {  10, "oneMicrodegreeEast" },
  { -10, "oneMicrodegreeWest" },
  { 1800000001, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_Longitude(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -1800000000, 1800000001U, NULL, FALSE);

  return offset;
}


static const value_string its_SemiAxisLength_vals[] = {
  {   1, "oneCentimeter" },
  { 4094, "outOfRange" },
  { 4095, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_SemiAxisLength(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 4095U, NULL, FALSE);

  return offset;
}


static const value_string its_HeadingValue_vals[] = {
  {   0, "wgs84North" },
  { 900, "wgs84East" },
  { 1800, "wgs84South" },
  { 2700, "wgs84West" },
  { 3601, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_HeadingValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 3601U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_PosConfidenceEllipse_sequence[] = {
  { &hf_its_semiMajorConfidence, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_SemiAxisLength },
  { &hf_its_semiMinorConfidence, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_SemiAxisLength },
  { &hf_its_semiMajorOrientation, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_HeadingValue },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_PosConfidenceEllipse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_PosConfidenceEllipse, its_PosConfidenceEllipse_sequence);

  return offset;
}


static const value_string its_AltitudeValue_vals[] = {
  {   0, "referenceEllipsoidSurface" },
  {   1, "oneCentimeter" },
  { 800001, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_AltitudeValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -100000, 800001U, NULL, FALSE);

  return offset;
}


static const value_string its_AltitudeConfidence_vals[] = {
  {   0, "alt-000-01" },
  {   1, "alt-000-02" },
  {   2, "alt-000-05" },
  {   3, "alt-000-10" },
  {   4, "alt-000-20" },
  {   5, "alt-000-50" },
  {   6, "alt-001-00" },
  {   7, "alt-002-00" },
  {   8, "alt-005-00" },
  {   9, "alt-010-00" },
  {  10, "alt-020-00" },
  {  11, "alt-050-00" },
  {  12, "alt-100-00" },
  {  13, "alt-200-00" },
  {  14, "outOfRange" },
  {  15, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_AltitudeConfidence(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     16, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t its_Altitude_sequence[] = {
  { &hf_its_altitudeValue   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_AltitudeValue },
  { &hf_its_altitudeConfidence, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_AltitudeConfidence },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_Altitude(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_Altitude, its_Altitude_sequence);

  return offset;
}


static const per_sequence_t its_ReferencePosition_sequence[] = {
  { &hf_its_latitude        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Latitude },
  { &hf_its_longitude       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Longitude },
  { &hf_its_positionConfidenceEllipse, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PosConfidenceEllipse },
  { &hf_its_altitude        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Altitude },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_ReferencePosition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_ReferencePosition, its_ReferencePosition_sequence);

  return offset;
}


static const value_string its_DeltaLatitude_vals[] = {
  {  10, "oneMicrodegreeNorth" },
  { -10, "oneMicrodegreeSouth" },
  { 131072, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_DeltaLatitude(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -131071, 131072U, NULL, FALSE);

  return offset;
}


static const value_string its_DeltaLongitude_vals[] = {
  {  10, "oneMicrodegreeEast" },
  { -10, "oneMicrodegreeWest" },
  { 131072, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_DeltaLongitude(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -131071, 131072U, NULL, FALSE);

  return offset;
}


static const value_string its_DeltaAltitude_vals[] = {
  {   1, "oneCentimeterUp" },
  {  -1, "oneCentimeterDown" },
  { 12800, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_DeltaAltitude(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -12700, 12800U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_DeltaReferencePosition_sequence[] = {
  { &hf_its_deltaLatitude   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DeltaLatitude },
  { &hf_its_deltaLongitude  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DeltaLongitude },
  { &hf_its_deltaAltitude   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DeltaAltitude },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_DeltaReferencePosition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_DeltaReferencePosition, its_DeltaReferencePosition_sequence);

  return offset;
}


static const value_string its_PathDeltaTime_vals[] = {
  {   1, "tenMilliSecondsInPast" },
  { 0, NULL }
};


static int
dissect_its_PathDeltaTime(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 65535U, NULL, TRUE);

  return offset;
}


static const per_sequence_t its_PathPoint_sequence[] = {
  { &hf_its_pathPosition    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DeltaReferencePosition },
  { &hf_its_pathDeltaTime   , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_PathDeltaTime },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_PathPoint(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_PathPoint, its_PathPoint_sequence);

  return offset;
}


static const value_string its_PtActivationType_vals[] = {
  {   0, "undefinedCodingType" },
  {   1, "r09-16CodingType" },
  {   2, "vdv-50149CodingType" },
  { 0, NULL }
};


static int
dissect_its_PtActivationType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}



static int
dissect_its_PtActivationData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       1, 20, FALSE, NULL);

  return offset;
}


static const per_sequence_t its_PtActivation_sequence[] = {
  { &hf_its_ptActivationType, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PtActivationType },
  { &hf_its_ptActivationData, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PtActivationData },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_PtActivation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_PtActivation, its_PtActivation_sequence);

  return offset;
}



static int
dissect_its_AccelerationControl(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 681 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     7, 7, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_its_AccelerationControl_brakePedalEngaged,
      &hf_its_AccelerationControl_gasPedalEngaged,
      &hf_its_AccelerationControl_emergencyBrakeEngaged,
      &hf_its_AccelerationControl_collisionWarningEngaged,
      &hf_its_AccelerationControl_accEngaged,
      &hf_its_AccelerationControl_cruiseControlEngaged,
      &hf_its_AccelerationControl_speedLimiterEngaged,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_its_AccelerationControl);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const value_string its_CauseCodeType_vals[] = {
  { reserved, "reserved" },
  { trafficCondition, "trafficCondition" },
  { accident, "accident" },
  { roadworks, "roadworks" },
  { impassability, "impassability" },
  { adverseWeatherCondition_Adhesion, "adverseWeatherCondition-Adhesion" },
  { aquaplannning, "aquaplannning" },
  { hazardousLocation_SurfaceCondition, "hazardousLocation-SurfaceCondition" },
  { hazardousLocation_ObstacleOnTheRoad, "hazardousLocation-ObstacleOnTheRoad" },
  { hazardousLocation_AnimalOnTheRoad, "hazardousLocation-AnimalOnTheRoad" },
  { humanPresenceOnTheRoad, "humanPresenceOnTheRoad" },
  { wrongWayDriving, "wrongWayDriving" },
  { rescueAndRecoveryWorkInProgress, "rescueAndRecoveryWorkInProgress" },
  { adverseWeatherCondition_ExtremeWeatherCondition, "adverseWeatherCondition-ExtremeWeatherCondition" },
  { adverseWeatherCondition_Visibility, "adverseWeatherCondition-Visibility" },
  { adverseWeatherCondition_Precipitation, "adverseWeatherCondition-Precipitation" },
  { slowVehicle, "slowVehicle" },
  { dangerousEndOfQueue, "dangerousEndOfQueue" },
  { vehicleBreakdown, "vehicleBreakdown" },
  { postCrash, "postCrash" },
  { humanProblem, "humanProblem" },
  { stationaryVehicle, "stationaryVehicle" },
  { emergencyVehicleApproaching, "emergencyVehicleApproaching" },
  { hazardousLocation_DangerousCurve, "hazardousLocation-DangerousCurve" },
  { collisionRisk, "collisionRisk" },
  { signalViolation, "signalViolation" },
  { dangerousSituation, "dangerousSituation" },
  { 0, NULL }
};


static int
dissect_its_CauseCodeType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, &((its_private_data_t*)actx->private_data)->cause_code, FALSE);

  return offset;
}



static int
dissect_its_SubCauseCodeType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1180 "./asn1/its/its.cnf"
  // Overwrite hf_index
  hf_index = *find_subcause_from_cause((CauseCodeType_enum) ((its_private_data_t*)actx->private_data)->cause_code);
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);




  return offset;
}


static const per_sequence_t its_CauseCode_sequence[] = {
  { &hf_its_causeCode       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_CauseCodeType },
  { &hf_its_subCauseCode    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_SubCauseCodeType },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_CauseCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_CauseCode, its_CauseCode_sequence);

  return offset;
}


static const value_string its_TrafficConditionSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "increasedVolumeOfTraffic" },
  {   2, "trafficJamSlowlyIncreasing" },
  {   3, "trafficJamIncreasing" },
  {   4, "trafficJamStronglyIncreasing" },
  {   5, "trafficStationary" },
  {   6, "trafficJamSlightlyDecreasing" },
  {   7, "trafficJamDecreasing" },
  {   8, "trafficJamStronglyDecreasing" },
  { 0, NULL }
};


static const value_string its_AccidentSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "multiVehicleAccident" },
  {   2, "heavyAccident" },
  {   3, "accidentInvolvingLorry" },
  {   4, "accidentInvolvingBus" },
  {   5, "accidentInvolvingHazardousMaterials" },
  {   6, "accidentOnOppositeLane" },
  {   7, "unsecuredAccident" },
  {   8, "assistanceRequested" },
  { 0, NULL }
};


static const value_string its_RoadworksSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "majorRoadworks" },
  {   2, "roadMarkingWork" },
  {   3, "slowMovingRoadMaintenance" },
  {   4, "shortTermStationaryRoadworks" },
  {   5, "streetCleaning" },
  {   6, "winterService" },
  { 0, NULL }
};


static int
dissect_its_RoadworksSubCauseCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const value_string its_HumanPresenceOnTheRoadSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "childrenOnRoadway" },
  {   2, "cyclistOnRoadway" },
  {   3, "motorcyclistOnRoadway" },
  { 0, NULL }
};


static const value_string its_WrongWayDrivingSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "wrongLane" },
  {   2, "wrongDirection" },
  { 0, NULL }
};


static const value_string its_AdverseWeatherCondition_ExtremeWeatherConditionSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "strongWinds" },
  {   2, "damagingHail" },
  {   3, "hurricane" },
  {   4, "thunderstorm" },
  {   5, "tornado" },
  {   6, "blizzard" },
  { 0, NULL }
};


static const value_string its_AdverseWeatherCondition_AdhesionSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "heavyFrostOnRoad" },
  {   2, "fuelOnRoad" },
  {   3, "mudOnRoad" },
  {   4, "snowOnRoad" },
  {   5, "iceOnRoad" },
  {   6, "blackIceOnRoad" },
  {   7, "oilOnRoad" },
  {   8, "looseChippings" },
  {   9, "instantBlackIce" },
  {  10, "roadsSalted" },
  { 0, NULL }
};


static const value_string its_AdverseWeatherCondition_VisibilitySubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "fog" },
  {   2, "smoke" },
  {   3, "heavySnowfall" },
  {   4, "heavyRain" },
  {   5, "heavyHail" },
  {   6, "lowSunGlare" },
  {   7, "sandstorms" },
  {   8, "swarmsOfInsects" },
  { 0, NULL }
};


static const value_string its_AdverseWeatherCondition_PrecipitationSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "heavyRain" },
  {   2, "heavySnowfall" },
  {   3, "softHail" },
  { 0, NULL }
};


static const value_string its_SlowVehicleSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "maintenanceVehicle" },
  {   2, "vehiclesSlowingToLookAtAccident" },
  {   3, "abnormalLoad" },
  {   4, "abnormalWideLoad" },
  {   5, "convoy" },
  {   6, "snowplough" },
  {   7, "deicing" },
  {   8, "saltingVehicles" },
  { 0, NULL }
};


static const value_string its_StationaryVehicleSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "humanProblem" },
  {   2, "vehicleBreakdown" },
  {   3, "postCrash" },
  {   4, "publicTransportStop" },
  {   5, "carryingDangerousGoods" },
  { 0, NULL }
};


static const value_string its_HumanProblemSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "glycemiaProblem" },
  {   2, "heartProblem" },
  { 0, NULL }
};


static const value_string its_EmergencyVehicleApproachingSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "emergencyVehicleApproaching" },
  {   2, "prioritizedVehicleApproaching" },
  { 0, NULL }
};


static const value_string its_HazardousLocation_DangerousCurveSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "dangerousLeftTurnCurve" },
  {   2, "dangerousRightTurnCurve" },
  {   3, "multipleCurvesStartingWithUnknownTurningDirection" },
  {   4, "multipleCurvesStartingWithLeftTurn" },
  {   5, "multipleCurvesStartingWithRightTurn" },
  { 0, NULL }
};


static const value_string its_HazardousLocation_SurfaceConditionSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "rockfalls" },
  {   2, "earthquakeDamage" },
  {   3, "sewerCollapse" },
  {   4, "subsidence" },
  {   5, "snowDrifts" },
  {   6, "stormDamage" },
  {   7, "burstPipe" },
  {   8, "volcanoEruption" },
  {   9, "fallingIce" },
  { 0, NULL }
};


static const value_string its_HazardousLocation_ObstacleOnTheRoadSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "shedLoad" },
  {   2, "partsOfVehicles" },
  {   3, "partsOfTyres" },
  {   4, "bigObjects" },
  {   5, "fallenTrees" },
  {   6, "hubCaps" },
  {   7, "waitingVehicles" },
  { 0, NULL }
};


static const value_string its_HazardousLocation_AnimalOnTheRoadSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "wildAnimals" },
  {   2, "herdOfAnimals" },
  {   3, "smallAnimals" },
  {   4, "largeAnimals" },
  { 0, NULL }
};


static const value_string its_CollisionRiskSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "longitudinalCollisionRisk" },
  {   2, "crossingCollisionRisk" },
  {   3, "lateralCollisionRisk" },
  {   4, "vulnerableRoadUser" },
  { 0, NULL }
};


static const value_string its_SignalViolationSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "stopSignViolation" },
  {   2, "trafficLightViolation" },
  {   3, "turningRegulationViolation" },
  { 0, NULL }
};


static const value_string its_RescueAndRecoveryWorkInProgressSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "emergencyVehicles" },
  {   2, "rescueHelicopterLanding" },
  {   3, "policeActivityOngoing" },
  {   4, "medicalEmergencyOngoing" },
  {   5, "childAbductionInProgress" },
  { 0, NULL }
};


static const value_string its_DangerousEndOfQueueSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "suddenEndOfQueue" },
  {   2, "queueOverHill" },
  {   3, "queueAroundBend" },
  {   4, "queueInTunnel" },
  { 0, NULL }
};


static const value_string its_DangerousSituationSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "emergencyElectronicBrakeEngaged" },
  {   2, "preCrashSystemEngaged" },
  {   3, "espEngaged" },
  {   4, "absEngaged" },
  {   5, "aebEngaged" },
  {   6, "brakeWarningEngaged" },
  {   7, "collisionRiskWarningEngaged" },
  { 0, NULL }
};


static const value_string its_VehicleBreakdownSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "lackOfFuel" },
  {   2, "lackOfBatteryPower" },
  {   3, "engineProblem" },
  {   4, "transmissionProblem" },
  {   5, "engineCoolingProblem" },
  {   6, "brakingSystemProblem" },
  {   7, "steeringProblem" },
  {   8, "tyrePuncture" },
  {   9, "tyrePressureProblem" },
  { 0, NULL }
};


static const value_string its_PostCrashSubCauseCode_vals[] = {
  {   0, "unavailable" },
  {   1, "accidentWithoutECallTriggered" },
  {   2, "accidentWithECallManuallyTriggered" },
  {   3, "accidentWithECallAutomaticallyTriggered" },
  {   4, "accidentWithECallTriggeredWithoutAccessToCellularNetwork" },
  { 0, NULL }
};


static const value_string its_CurvatureValue_vals[] = {
  {   0, "straight" },
  { 1023, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_CurvatureValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -1023, 1023U, NULL, FALSE);

  return offset;
}


static const value_string its_CurvatureConfidence_vals[] = {
  {   0, "onePerMeter-0-00002" },
  {   1, "onePerMeter-0-0001" },
  {   2, "onePerMeter-0-0005" },
  {   3, "onePerMeter-0-002" },
  {   4, "onePerMeter-0-01" },
  {   5, "onePerMeter-0-1" },
  {   6, "outOfRange" },
  {   7, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_CurvatureConfidence(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     8, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t its_Curvature_sequence[] = {
  { &hf_its_curvatureValue  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_CurvatureValue },
  { &hf_its_curvatureConfidence, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_CurvatureConfidence },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_Curvature(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_Curvature, its_Curvature_sequence);

  return offset;
}


static const value_string its_CurvatureCalculationMode_vals[] = {
  {   0, "yawRateUsed" },
  {   1, "yawRateNotUsed" },
  {   2, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_CurvatureCalculationMode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string its_HeadingConfidence_vals[] = {
  {   1, "equalOrWithinZeroPointOneDegree" },
  {  10, "equalOrWithinOneDegree" },
  { 126, "outOfRange" },
  { 127, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_HeadingConfidence(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 127U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_Heading_sequence[] = {
  { &hf_its_headingValue    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_HeadingValue },
  { &hf_its_headingConfidence, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_HeadingConfidence },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_Heading(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_Heading, its_Heading_sequence);

  return offset;
}


static const value_string its_LanePosition_vals[] = {
  {  -1, "offTheRoad" },
  {   0, "hardShoulder" },
  {   1, "outermostDrivingLane" },
  {   2, "secondLaneFromOutside" },
  { 0, NULL }
};


static int
dissect_its_LanePosition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -1, 14U, NULL, FALSE);

  return offset;
}


static const value_string its_HardShoulderStatus_vals[] = {
  {   0, "availableForStopping" },
  {   1, "closed" },
  {   2, "availableForDriving" },
  { 0, NULL }
};


static int
dissect_its_HardShoulderStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_its_DrivingLaneStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     1, 13, FALSE, NULL, NULL);

  return offset;
}


static const per_sequence_t its_ClosedLanes_sequence[] = {
  { &hf_its_innerhardShoulderStatus, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_HardShoulderStatus },
  { &hf_its_outerhardShoulderStatus, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_HardShoulderStatus },
  { &hf_its_drivingLaneStatus, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_DrivingLaneStatus },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_ClosedLanes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_ClosedLanes, its_ClosedLanes_sequence);

  return offset;
}


static const value_string its_PerformanceClass_vals[] = {
  {   0, "unavailable" },
  {   1, "performanceClassA" },
  {   2, "performanceClassB" },
  { 0, NULL }
};


static int
dissect_its_PerformanceClass(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}


static const value_string its_SpeedValue_vals[] = {
  {   0, "standstill" },
  {   1, "oneCentimeterPerSec" },
  { 16383, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_SpeedValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 16383U, NULL, FALSE);

  return offset;
}


static const value_string its_SpeedConfidence_vals[] = {
  {   1, "equalOrWithinOneCentimeterPerSec" },
  { 100, "equalOrWithinOneMeterPerSec" },
  { 126, "outOfRange" },
  { 127, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_SpeedConfidence(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 127U, NULL, FALSE);

  return offset;
}


static const value_string its_VehicleMass_vals[] = {
  {   1, "hundredKg" },
  { 1024, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_VehicleMass(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 1024U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_Speed_sequence[] = {
  { &hf_its_speedValue      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_SpeedValue },
  { &hf_its_speedConfidence , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_SpeedConfidence },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_Speed(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_Speed, its_Speed_sequence);

  return offset;
}


static const value_string its_DriveDirection_vals[] = {
  {   0, "forward" },
  {   1, "backward" },
  {   2, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_DriveDirection(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_its_EmbarkationStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_boolean(tvb, offset, actx, tree, hf_index, NULL);

  return offset;
}


static const value_string its_LongitudinalAccelerationValue_vals[] = {
  {   1, "pointOneMeterPerSecSquaredForward" },
  {  -1, "pointOneMeterPerSecSquaredBackward" },
  { 161, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_LongitudinalAccelerationValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -160, 161U, NULL, FALSE);

  return offset;
}


static const value_string its_AccelerationConfidence_vals[] = {
  {   1, "pointOneMeterPerSecSquared" },
  { 101, "outOfRange" },
  { 102, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_AccelerationConfidence(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 102U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_LongitudinalAcceleration_sequence[] = {
  { &hf_its_longitudinalAccelerationValue, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_LongitudinalAccelerationValue },
  { &hf_its_longitudinalAccelerationConfidence, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_AccelerationConfidence },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_LongitudinalAcceleration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_LongitudinalAcceleration, its_LongitudinalAcceleration_sequence);

  return offset;
}


static const value_string its_LateralAccelerationValue_vals[] = {
  {  -1, "pointOneMeterPerSecSquaredToRight" },
  {   1, "pointOneMeterPerSecSquaredToLeft" },
  { 161, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_LateralAccelerationValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -160, 161U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_LateralAcceleration_sequence[] = {
  { &hf_its_lateralAccelerationValue, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_LateralAccelerationValue },
  { &hf_its_lateralAccelerationConfidence, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_AccelerationConfidence },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_LateralAcceleration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_LateralAcceleration, its_LateralAcceleration_sequence);

  return offset;
}


static const value_string its_VerticalAccelerationValue_vals[] = {
  {   1, "pointOneMeterPerSecSquaredUp" },
  {  -1, "pointOneMeterPerSecSquaredDown" },
  { 161, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_VerticalAccelerationValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -160, 161U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_VerticalAcceleration_sequence[] = {
  { &hf_its_verticalAccelerationValue, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_VerticalAccelerationValue },
  { &hf_its_verticalAccelerationConfidence, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_AccelerationConfidence },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_VerticalAcceleration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_VerticalAcceleration, its_VerticalAcceleration_sequence);

  return offset;
}


static const value_string its_StationType_vals[] = {
  {   0, "unknown" },
  {   1, "pedestrian" },
  {   2, "cyclist" },
  {   3, "moped" },
  {   4, "motorcycle" },
  {   5, "passengerCar" },
  {   6, "bus" },
  {   7, "lightTruck" },
  {   8, "heavyTruck" },
  {   9, "trailer" },
  {  10, "specialVehicles" },
  {  11, "tram" },
  {  15, "roadSideUnit" },
  { 0, NULL }
};


static int
dissect_its_StationType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}



static int
dissect_its_ExteriorLights(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 966 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     8, 8, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_its_ExteriorLights_lowBeamHeadlightsOn,
      &hf_its_ExteriorLights_highBeamHeadlightsOn,
      &hf_its_ExteriorLights_leftTurnSignalOn,
      &hf_its_ExteriorLights_rightTurnSignalOn,
      &hf_its_ExteriorLights_daytimeRunningLightsOn,
      &hf_its_ExteriorLights_reverseLightOn,
      &hf_its_ExteriorLights_fogLightOn,
      &hf_its_ExteriorLights_parkingLightsOn,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_its_ExteriorLights);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const value_string its_DangerousGoodsBasic_vals[] = {
  {   0, "explosives1" },
  {   1, "explosives2" },
  {   2, "explosives3" },
  {   3, "explosives4" },
  {   4, "explosives5" },
  {   5, "explosives6" },
  {   6, "flammableGases" },
  {   7, "nonFlammableGases" },
  {   8, "toxicGases" },
  {   9, "flammableLiquids" },
  {  10, "flammableSolids" },
  {  11, "substancesLiableToSpontaneousCombustion" },
  {  12, "substancesEmittingFlammableGasesUponContactWithWater" },
  {  13, "oxidizingSubstances" },
  {  14, "organicPeroxides" },
  {  15, "toxicSubstances" },
  {  16, "infectiousSubstances" },
  {  17, "radioactiveMaterial" },
  {  18, "corrosiveSubstances" },
  {  19, "miscellaneousDangerousSubstances" },
  { 0, NULL }
};


static int
dissect_its_DangerousGoodsBasic(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     20, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_its_INTEGER_0_9999(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 9999U, NULL, FALSE);

  return offset;
}



static int
dissect_its_BOOLEAN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_boolean(tvb, offset, actx, tree, hf_index, NULL);

  return offset;
}



static int
dissect_its_IA5String_SIZE_1_24(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          1, 24, FALSE);

  return offset;
}



static int
dissect_its_PhoneNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_NumericString(tvb, offset, actx, tree, hf_index,
                                          1, 16, FALSE);

  return offset;
}



static int
dissect_its_UTF8String_SIZE_1_24(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 24, FALSE);

  return offset;
}


static const per_sequence_t its_DangerousGoodsExtended_sequence[] = {
  { &hf_its_dangerousGoodsType, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_DangerousGoodsBasic },
  { &hf_its_unNumber        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_INTEGER_0_9999 },
  { &hf_its_elevatedTemperature, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_BOOLEAN },
  { &hf_its_tunnelsRestricted, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_BOOLEAN },
  { &hf_its_limitedQuantity , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_BOOLEAN },
  { &hf_its_emergencyActionCode, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_IA5String_SIZE_1_24 },
  { &hf_its_phoneNumber     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_PhoneNumber },
  { &hf_its_companyName     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_UTF8String_SIZE_1_24 },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_DangerousGoodsExtended(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_DangerousGoodsExtended, its_DangerousGoodsExtended_sequence);

  return offset;
}



static int
dissect_its_SpecialTransportType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1098 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     4, 4, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_its_SpecialTransportType_heavyLoad,
      &hf_its_SpecialTransportType_excessWidth,
      &hf_its_SpecialTransportType_excessLength,
      &hf_its_SpecialTransportType_excessHeight,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_its_SpecialTransportType);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_its_LightBarSirenInUse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 758 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     2, 2, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_its_LightBarSirenInUse_lightBarActivated,
      &hf_its_LightBarSirenInUse_sirenActivated,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_its_LightBarSirenInUse);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const value_string its_HeightLonCarr_vals[] = {
  {   1, "oneCentimeter" },
  { 100, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_HeightLonCarr(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 100U, NULL, FALSE);

  return offset;
}


static const value_string its_PosLonCarr_vals[] = {
  {   1, "oneCentimeter" },
  { 127, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_PosLonCarr(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 127U, NULL, FALSE);

  return offset;
}


static const value_string its_PosPillar_vals[] = {
  {   1, "tenCentimeters" },
  {  30, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_PosPillar(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 30U, NULL, FALSE);

  return offset;
}


static const value_string its_PosCentMass_vals[] = {
  {   1, "tenCentimeters" },
  {  63, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_PosCentMass(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 63U, NULL, FALSE);

  return offset;
}


static const value_string its_RequestResponseIndication_vals[] = {
  {   0, "request" },
  {   1, "response" },
  { 0, NULL }
};


static int
dissect_its_RequestResponseIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string its_SpeedLimit_vals[] = {
  {   1, "oneKmPerHour" },
  { 0, NULL }
};


static int
dissect_its_SpeedLimit(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 255U, NULL, FALSE);

  return offset;
}


static const value_string its_StationarySince_vals[] = {
  {   0, "lessThan1Minute" },
  {   1, "lessThan2Minutes" },
  {   2, "lessThan15Minutes" },
  {   3, "equalOrGreater15Minutes" },
  { 0, NULL }
};


static int
dissect_its_StationarySince(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string its_Temperature_vals[] = {
  { -60, "equalOrSmallerThanMinus60Deg" },
  {   1, "oneDegreeCelsius" },
  {  67, "equalOrGreaterThan67Deg" },
  { 0, NULL }
};


static int
dissect_its_Temperature(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -60, 67U, NULL, FALSE);

  return offset;
}


static const value_string its_TrafficRule_vals[] = {
  {   0, "noPassing" },
  {   1, "noPassingForTrucks" },
  {   2, "passToRight" },
  {   3, "passToLeft" },
  { 0, NULL }
};


static int
dissect_its_TrafficRule(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string its_WheelBaseVehicle_vals[] = {
  {   1, "tenCentimeters" },
  { 127, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_WheelBaseVehicle(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 127U, NULL, FALSE);

  return offset;
}


static const value_string its_TurningRadius_vals[] = {
  {   1, "point4Meters" },
  { 255, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_TurningRadius(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 255U, NULL, FALSE);

  return offset;
}


static const value_string its_PosFrontAx_vals[] = {
  {   1, "tenCentimeters" },
  {  20, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_PosFrontAx(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 20U, NULL, FALSE);

  return offset;
}



static int
dissect_its_PositionOfOccupants(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1059 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     20, 20, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields0[] = { // CHECKED
      &hf_its_PositionOfOccupants_row1LeftOccupied,
      &hf_its_PositionOfOccupants_row1RightOccupied,
      &hf_its_PositionOfOccupants_row1MidOccupied,
      &hf_its_PositionOfOccupants_row1NotDetectable,
      &hf_its_PositionOfOccupants_row1NotPresent,
      &hf_its_PositionOfOccupants_row2LeftOccupied,
      &hf_its_PositionOfOccupants_row2RightOccupied,
      &hf_its_PositionOfOccupants_row2MidOccupied,
      NULL
    };
    const gint *fields1[] = { // CHECKED
      &hf_its_PositionOfOccupants_row2NotDetectable,
      &hf_its_PositionOfOccupants_row2NotPresent,
      &hf_its_PositionOfOccupants_row3LeftOccupied,
      &hf_its_PositionOfOccupants_row3RightOccupied,
      &hf_its_PositionOfOccupants_row3MidOccupied,
      &hf_its_PositionOfOccupants_row3NotDetectable,
      &hf_its_PositionOfOccupants_row3NotPresent,
      &hf_its_PositionOfOccupants_row4LeftOccupied,
      NULL
    };
    const gint *fields2[] = { // CHECKED
      &hf_its_PositionOfOccupants_row4RightOccupied,
      &hf_its_PositionOfOccupants_row4MidOccupied,
      &hf_its_PositionOfOccupants_row4NotDetectable,
      &hf_its_PositionOfOccupants_row4NotPresent,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_its_PositionOfOccupants);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields0, ENC_BIG_ENDIAN);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 1, 1, fields1, ENC_BIG_ENDIAN);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 2, 1, fields2, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const value_string its_PositioningSolutionType_vals[] = {
  {   0, "noPositioningSolution" },
  {   1, "sGNSS" },
  {   2, "dGNSS" },
  {   3, "sGNSSplusDR" },
  {   4, "dGNSSplusDR" },
  {   5, "dR" },
  { 0, NULL }
};


static int
dissect_its_PositioningSolutionType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     6, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_its_WMInumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          1, 3, FALSE);

  return offset;
}



static int
dissect_its_VDS(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          6, 6, FALSE);

  return offset;
}


static const per_sequence_t its_VehicleIdentification_sequence[] = {
  { &hf_its_wMInumber       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_WMInumber },
  { &hf_its_vDS             , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_VDS },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_VehicleIdentification(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_VehicleIdentification, its_VehicleIdentification_sequence);

  return offset;
}



static int
dissect_its_EnergyStorageType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 806 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     7, 7, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_its_EnergyStorageType_hydrogenStorage,
      &hf_its_EnergyStorageType_electricEnergyStorage,
      &hf_its_EnergyStorageType_liquidPropaneGas,
      &hf_its_EnergyStorageType_compressedNaturalGas,
      &hf_its_EnergyStorageType_diesel,
      &hf_its_EnergyStorageType_gasoline,
      &hf_its_EnergyStorageType_ammonia,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_its_EnergyStorageType);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const value_string its_VehicleLengthValue_vals[] = {
  {   1, "tenCentimeters" },
  { 1022, "outOfRange" },
  { 1023, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_VehicleLengthValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 1023U, NULL, FALSE);

  return offset;
}


static const value_string its_VehicleLengthConfidenceIndication_vals[] = {
  {   0, "noTrailerPresent" },
  {   1, "trailerPresentWithKnownLength" },
  {   2, "trailerPresentWithUnknownLength" },
  {   3, "trailerPresenceIsUnknown" },
  {   4, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_VehicleLengthConfidenceIndication(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     5, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t its_VehicleLength_sequence[] = {
  { &hf_its_vehicleLengthValue, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_VehicleLengthValue },
  { &hf_its_vehicleLengthConfidenceIndication, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_VehicleLengthConfidenceIndication },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_VehicleLength(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_VehicleLength, its_VehicleLength_sequence);

  return offset;
}


static const value_string its_VehicleWidth_vals[] = {
  {   1, "tenCentimeters" },
  {  61, "outOfRange" },
  {  62, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_VehicleWidth(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 62U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_PathHistory_sequence_of[1] = {
  { &hf_its_PathHistory_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PathPoint },
};

static int
dissect_its_PathHistory(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_its_PathHistory, its_PathHistory_sequence_of,
                                                  0, 40, FALSE);

  return offset;
}



static int
dissect_its_EmergencyPriority(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 824 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     2, 2, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_its_EmergencyPriority_requestForRightOfWay,
      &hf_its_EmergencyPriority_requestForFreeCrossingAtATrafficLight,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_its_EmergencyPriority);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const value_string its_InformationQuality_vals[] = {
  {   0, "unavailable" },
  {   1, "lowest" },
  {   7, "highest" },
  { 0, NULL }
};


static int
dissect_its_InformationQuality(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}


static const value_string its_RoadType_vals[] = {
  {   0, "urban-NoStructuralSeparationToOppositeLanes" },
  {   1, "urban-WithStructuralSeparationToOppositeLanes" },
  {   2, "nonUrban-NoStructuralSeparationToOppositeLanes" },
  {   3, "nonUrban-WithStructuralSeparationToOppositeLanes" },
  { 0, NULL }
};


static int
dissect_its_RoadType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string its_SteeringWheelAngleValue_vals[] = {
  {   0, "straight" },
  {  -1, "onePointFiveDegreesToRight" },
  {   1, "onePointFiveDegreesToLeft" },
  { 512, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_SteeringWheelAngleValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -511, 512U, NULL, FALSE);

  return offset;
}


static const value_string its_SteeringWheelAngleConfidence_vals[] = {
  {   1, "equalOrWithinOnePointFiveDegree" },
  { 126, "outOfRange" },
  { 127, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_SteeringWheelAngleConfidence(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 127U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_SteeringWheelAngle_sequence[] = {
  { &hf_its_steeringWheelAngleValue, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_SteeringWheelAngleValue },
  { &hf_its_steeringWheelAngleConfidence, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_SteeringWheelAngleConfidence },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_SteeringWheelAngle(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_SteeringWheelAngle, its_SteeringWheelAngle_sequence);

  return offset;
}



static int
dissect_its_TimestampIts(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer_64b(tvb, offset, actx, tree, hf_index,
                                                            0U, G_GUINT64_CONSTANT(4398046511103), NULL, FALSE);

  return offset;
}


static const value_string its_VehicleRole_vals[] = {
  {   0, "default" },
  {   1, "publicTransport" },
  {   2, "specialTransport" },
  {   3, "dangerousGoods" },
  {   4, "roadWork" },
  {   5, "rescue" },
  {   6, "emergency" },
  {   7, "safetyCar" },
  {   8, "agriculture" },
  {   9, "commercial" },
  {  10, "military" },
  {  11, "roadOperator" },
  {  12, "taxi" },
  {  13, "reserved1" },
  {  14, "reserved2" },
  {  15, "reserved3" },
  { 0, NULL }
};


static int
dissect_its_VehicleRole(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     16, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string its_YawRateValue_vals[] = {
  {   0, "straight" },
  {  -1, "degSec-000-01ToRight" },
  {   1, "degSec-000-01ToLeft" },
  { 32767, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_YawRateValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -32766, 32767U, NULL, FALSE);

  return offset;
}


static const value_string its_YawRateConfidence_vals[] = {
  {   0, "degSec-000-01" },
  {   1, "degSec-000-05" },
  {   2, "degSec-000-10" },
  {   3, "degSec-001-00" },
  {   4, "degSec-005-00" },
  {   5, "degSec-010-00" },
  {   6, "degSec-100-00" },
  {   7, "outOfRange" },
  {   8, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_YawRateConfidence(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     9, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t its_YawRate_sequence[] = {
  { &hf_its_yawRateValue    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_YawRateValue },
  { &hf_its_yawRateConfidence, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_YawRateConfidence },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_YawRate(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_YawRate, its_YawRate_sequence);

  return offset;
}


static const value_string its_ProtectedZoneType_vals[] = {
  {   0, "permanentCenDsrcTolling" },
  {   1, "temporaryCenDsrcTolling" },
  { 0, NULL }
};


static int
dissect_its_ProtectedZoneType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 1, NULL);

  return offset;
}


static const value_string its_RelevanceDistance_vals[] = {
  {   0, "lessThan50m" },
  {   1, "lessThan100m" },
  {   2, "lessThan200m" },
  {   3, "lessThan500m" },
  {   4, "lessThan1000m" },
  {   5, "lessThan5km" },
  {   6, "lessThan10km" },
  {   7, "over10km" },
  { 0, NULL }
};


static int
dissect_its_RelevanceDistance(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     8, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string its_RelevanceTrafficDirection_vals[] = {
  {   0, "allTrafficDirections" },
  {   1, "upstreamTraffic" },
  {   2, "downstreamTraffic" },
  {   3, "oppositeTraffic" },
  { 0, NULL }
};


static int
dissect_its_RelevanceTrafficDirection(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string its_TransmissionInterval_vals[] = {
  {   1, "oneMilliSecond" },
  { 10000, "tenSeconds" },
  { 0, NULL }
};


static int
dissect_its_TransmissionInterval(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 10000U, NULL, FALSE);

  return offset;
}


static const value_string its_ValidityDuration_vals[] = {
  {   0, "timeOfDetection" },
  {   1, "oneSecondAfterDetection" },
  { 0, NULL }
};


static int
dissect_its_ValidityDuration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 86400U, NULL, FALSE);

  return offset;
}



static int
dissect_its_SequenceNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_ActionID_sequence[] = {
  { &hf_its_originatingStationID, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_StationID },
  { &hf_its_sequenceNumber  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_SequenceNumber },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_ActionID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_ActionID, its_ActionID_sequence);

  return offset;
}


static const per_sequence_t its_ItineraryPath_sequence_of[1] = {
  { &hf_its_ItineraryPath_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_ReferencePosition },
};

static int
dissect_its_ItineraryPath(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_its_ItineraryPath, its_ItineraryPath_sequence_of,
                                                  1, 40, FALSE);

  return offset;
}


static const value_string its_ProtectedZoneRadius_vals[] = {
  {   1, "oneMeter" },
  { 0, NULL }
};


static int
dissect_its_ProtectedZoneRadius(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 255U, NULL, TRUE);

  return offset;
}



static int
dissect_its_ProtectedZoneID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 134217727U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_ProtectedCommunicationZone_sequence[] = {
  { &hf_its_protectedZoneType, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_ProtectedZoneType },
  { &hf_its_expiryTime      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_TimestampIts },
  { &hf_its_protectedZoneLatitude, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Latitude },
  { &hf_its_protectedZoneLongitude, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Longitude },
  { &hf_its_protectedZoneRadius, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_ProtectedZoneRadius },
  { &hf_its_protectedZoneID , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_ProtectedZoneID },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_ProtectedCommunicationZone(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_ProtectedCommunicationZone, its_ProtectedCommunicationZone_sequence);

  return offset;
}


static const per_sequence_t its_Traces_sequence_of[1] = {
  { &hf_its_Traces_item     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PathHistory },
};

static int
dissect_its_Traces(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_its_Traces, its_Traces_sequence_of,
                                                  1, 7, FALSE);

  return offset;
}


static const value_string its_NumberOfOccupants_vals[] = {
  {   1, "oneOccupant" },
  { 127, "unavailable" },
  { 0, NULL }
};


static int
dissect_its_NumberOfOccupants(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 127U, NULL, FALSE);

  return offset;
}


static const per_sequence_t its_PositionOfPillars_sequence_of[1] = {
  { &hf_its_PositionOfPillars_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PosPillar },
};

static int
dissect_its_PositionOfPillars(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_its_PositionOfPillars, its_PositionOfPillars_sequence_of,
                                                  1, 3, TRUE);

  return offset;
}


static const per_sequence_t its_RestrictedTypes_sequence_of[1] = {
  { &hf_its_RestrictedTypes_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_StationType },
};

static int
dissect_its_RestrictedTypes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_its_RestrictedTypes, its_RestrictedTypes_sequence_of,
                                                  1, 3, TRUE);

  return offset;
}


static const per_sequence_t its_EventPoint_sequence[] = {
  { &hf_its_eventPosition   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DeltaReferencePosition },
  { &hf_its_eventDeltaTime  , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_PathDeltaTime },
  { &hf_its_informationQuality, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_InformationQuality },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_EventPoint(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_EventPoint, its_EventPoint_sequence);

  return offset;
}


static const per_sequence_t its_EventHistory_sequence_of[1] = {
  { &hf_its_EventHistory_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_EventPoint },
};

static int
dissect_its_EventHistory(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_its_EventHistory, its_EventHistory_sequence_of,
                                                  1, 23, FALSE);

  return offset;
}


static const per_sequence_t its_ProtectedCommunicationZonesRSU_sequence_of[1] = {
  { &hf_its_ProtectedCommunicationZonesRSU_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_ProtectedCommunicationZone },
};

static int
dissect_its_ProtectedCommunicationZonesRSU(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_its_ProtectedCommunicationZonesRSU, its_ProtectedCommunicationZonesRSU_sequence_of,
                                                  1, 16, FALSE);

  return offset;
}



static int
dissect_its_CenDsrcTollingZoneID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_its_ProtectedZoneID(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t its_CenDsrcTollingZone_sequence[] = {
  { &hf_its_protectedZoneLatitude, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Latitude },
  { &hf_its_protectedZoneLongitude, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Longitude },
  { &hf_its_cenDsrcTollingZoneID, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_CenDsrcTollingZoneID },
  { NULL, 0, 0, NULL }
};

static int
dissect_its_CenDsrcTollingZone(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_its_CenDsrcTollingZone, its_CenDsrcTollingZone_sequence);

  return offset;
}


static const per_sequence_t its_DigitalMap_sequence_of[1] = {
  { &hf_its_DigitalMap_item , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_ReferencePosition },
};

static int
dissect_its_DigitalMap(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_its_DigitalMap, its_DigitalMap_sequence_of,
                                                  1, 256, FALSE);

  return offset;
}



static int
dissect_its_OpeningDaysHours(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          NO_BOUND, NO_BOUND, FALSE);

  return offset;
}

/*--- PDUs ---*/

static int dissect_its_ItsPduHeader_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_its_ItsPduHeader(tvb, offset, &asn1_ctx, tree, hf_its_its_ItsPduHeader_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/* --- Module AVIAEINumberingAndDataStructures --- --- ---                    */



static int
dissect_anads_CountryCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     10, 10, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_anads_IssuerIdentifier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 16383U, NULL, FALSE);

  return offset;
}


/* --- Module ElectronicRegistrationIdentificationVehicleDataModule --- --- --- */


static const value_string erivdm_EuVehicleCategoryL_vals[] = {
  {   0, "l1" },
  {   1, "l2" },
  {   2, "l3" },
  {   3, "l4" },
  {   4, "l5" },
  {   5, "l6" },
  {   6, "l7" },
  { 0, NULL }
};


static int
dissect_erivdm_EuVehicleCategoryL(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     7, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string erivdm_EuVehicleCategoryM_vals[] = {
  {   0, "m1" },
  {   1, "m2" },
  {   2, "m3" },
  { 0, NULL }
};


static int
dissect_erivdm_EuVehicleCategoryM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string erivdm_EuVehicleCategoryN_vals[] = {
  {   0, "n1" },
  {   1, "n2" },
  {   2, "n3" },
  { 0, NULL }
};


static int
dissect_erivdm_EuVehicleCategoryN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string erivdm_EuVehicleCategoryO_vals[] = {
  {   0, "o1" },
  {   1, "o2" },
  {   2, "o3" },
  {   3, "o4" },
  { 0, NULL }
};


static int
dissect_erivdm_EuVehicleCategoryO(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_erivdm_NULL(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_null(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const value_string erivdm_EuVehicleCategoryCode_vals[] = {
  {   0, "euVehicleCategoryL" },
  {   1, "euVehicleCategoryM" },
  {   2, "euVehicleCategoryN" },
  {   3, "euVehicleCategoryO" },
  {   4, "euVehilcleCategoryT" },
  {   5, "euVehilcleCategoryG" },
  { 0, NULL }
};

static const per_choice_t erivdm_EuVehicleCategoryCode_choice[] = {
  {   0, &hf_erivdm_euVehicleCategoryL, ASN1_NO_EXTENSIONS     , dissect_erivdm_EuVehicleCategoryL },
  {   1, &hf_erivdm_euVehicleCategoryM, ASN1_NO_EXTENSIONS     , dissect_erivdm_EuVehicleCategoryM },
  {   2, &hf_erivdm_euVehicleCategoryN, ASN1_NO_EXTENSIONS     , dissect_erivdm_EuVehicleCategoryN },
  {   3, &hf_erivdm_euVehicleCategoryO, ASN1_NO_EXTENSIONS     , dissect_erivdm_EuVehicleCategoryO },
  {   4, &hf_erivdm_euVehilcleCategoryT, ASN1_NO_EXTENSIONS     , dissect_erivdm_NULL },
  {   5, &hf_erivdm_euVehilcleCategoryG, ASN1_NO_EXTENSIONS     , dissect_erivdm_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_erivdm_EuVehicleCategoryCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_erivdm_EuVehicleCategoryCode, erivdm_EuVehicleCategoryCode_choice,
                                 NULL);

  return offset;
}


static const value_string erivdm_Iso3833VehicleType_vals[] = {
  {   0, "passengerCar" },
  {   1, "saloon" },
  {   2, "convertibleSaloon" },
  {   3, "pullmanSaloon" },
  {   4, "stationWagon" },
  {   5, "truckStationWagon" },
  {   6, "coupe" },
  {   7, "convertible" },
  {   8, "multipurposePassengerCar" },
  {   9, "forwardControlPassengerCar" },
  {  10, "specialPassengerCar" },
  {  11, "bus" },
  {  12, "minibus" },
  {  13, "urbanBus" },
  {  14, "interurbanCoach" },
  {  15, "longDistanceCoach" },
  {  16, "articulatedBus" },
  {  17, "trolleyBus" },
  {  18, "specialBus" },
  {  19, "commercialVehicle" },
  {  20, "specialCommercialVehicle" },
  {  21, "specialVehicle" },
  {  22, "trailingTowingVehicle" },
  {  23, "semiTrailerTowingVehicle" },
  {  24, "trailer" },
  {  25, "busTrailer" },
  {  26, "generalPurposeTrailer" },
  {  27, "caravan" },
  {  28, "specialTrailer" },
  {  29, "semiTrailer" },
  {  30, "busSemiTrailer" },
  {  31, "generalPurposeSemiTrailer" },
  {  32, "specialSemiTrailer" },
  {  33, "roadTrain" },
  {  34, "passengerRoadTrain" },
  {  35, "articulatedRoadTrain" },
  {  36, "doubleRoadTrain" },
  {  37, "compositeRoadTrain" },
  {  38, "specialRoadTrain" },
  {  39, "moped" },
  {  40, "motorCycle" },
  { 0, NULL }
};


static int
dissect_erivdm_Iso3833VehicleType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


/* --- Module CITSapplMgmtIDs --- --- ---                                     */



static int
dissect_csmid_INTEGER_0_127(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 127U, NULL, FALSE);

  return offset;
}



static int
dissect_csmid_INTEGER_128_16511(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            128U, 16511U, NULL, FALSE);

  return offset;
}



static int
dissect_csmid_INTEGER_16512_2113663(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            16512U, 2113663U, NULL, FALSE);

  return offset;
}



static int
dissect_csmid_Ext3(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            2113664U, 270549119U, NULL, TRUE);

  return offset;
}


static const value_string csmid_Ext2_vals[] = {
  {   0, "content" },
  {   1, "extension" },
  { 0, NULL }
};

static const per_choice_t csmid_Ext2_choice[] = {
  {   0, &hf_csmid_e2Content     , ASN1_NO_EXTENSIONS     , dissect_csmid_INTEGER_16512_2113663 },
  {   1, &hf_csmid_e1Extension   , ASN1_NO_EXTENSIONS     , dissect_csmid_Ext3 },
  { 0, NULL, 0, NULL }
};

static int
dissect_csmid_Ext2(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_csmid_Ext2, csmid_Ext2_choice,
                                 NULL);

  return offset;
}


static const value_string csmid_Ext1_vals[] = {
  {   0, "content" },
  {   1, "extension" },
  { 0, NULL }
};

static const per_choice_t csmid_Ext1_choice[] = {
  {   0, &hf_csmid_e1Content     , ASN1_NO_EXTENSIONS     , dissect_csmid_INTEGER_128_16511 },
  {   1, &hf_csmid_e2Extension   , ASN1_NO_EXTENSIONS     , dissect_csmid_Ext2 },
  { 0, NULL, 0, NULL }
};

static int
dissect_csmid_Ext1(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_csmid_Ext1, csmid_Ext1_choice,
                                 NULL);

  return offset;
}


static const value_string csmid_VarLengthNumber_vals[] = {
  {   0, "content" },
  {   1, "extension" },
  { 0, NULL }
};

static const per_choice_t csmid_VarLengthNumber_choice[] = {
  {   0, &hf_csmid_vlnContent    , ASN1_NO_EXTENSIONS     , dissect_csmid_INTEGER_0_127 },
  {   1, &hf_csmid_vlnExtension  , ASN1_NO_EXTENSIONS     , dissect_csmid_Ext1 },
  { 0, NULL, 0, NULL }
};

static int
dissect_csmid_VarLengthNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_csmid_VarLengthNumber, csmid_VarLengthNumber_choice,
                                 NULL);

  return offset;
}


/* --- Module EfcDsrcApplication --- --- ---                                  */



static int
dissect_dsrc_app_Int2(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_app_AxleWeightLimits_sequence[] = {
  { &hf_dsrc_app_maxLadenweightOnAxle1, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { &hf_dsrc_app_maxLadenweightOnAxle2, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { &hf_dsrc_app_maxLadenweightOnAxle3, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { &hf_dsrc_app_maxLadenweightOnAxle4, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { &hf_dsrc_app_maxLadenweightOnAxle5, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_app_AxleWeightLimits(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_app_AxleWeightLimits, dsrc_app_AxleWeightLimits_sequence);

  return offset;
}


static const value_string dsrc_app_UnitType_vals[] = {
  {   0, "mg-km" },
  {   1, "mg-kWh" },
  { 0, NULL }
};


static int
dissect_dsrc_app_UnitType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_dsrc_app_INTEGER_0_32767(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 32767U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_app_T_particulate_sequence[] = {
  { &hf_dsrc_app_unitType   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_UnitType },
  { &hf_dsrc_app_value      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_INTEGER_0_32767 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_app_T_particulate(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_app_T_particulate, dsrc_app_T_particulate_sequence);

  return offset;
}


static const per_sequence_t dsrc_app_DieselEmissionValues_sequence[] = {
  { &hf_dsrc_app_particulate, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_T_particulate },
  { &hf_dsrc_app_absorptionCoeff, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_app_DieselEmissionValues(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_app_DieselEmissionValues, dsrc_app_DieselEmissionValues_sequence);

  return offset;
}


static const value_string dsrc_app_EuroValue_vals[] = {
  {   0, "noEntry" },
  {   1, "euro-1" },
  {   2, "euro-2" },
  {   3, "euro-3" },
  {   4, "euro-4" },
  {   5, "euro-5" },
  {   6, "euro-6" },
  {   7, "reservedForUse1" },
  {   8, "reservedForUse2" },
  {   9, "reservedForUse3" },
  {  10, "reservedForUse4" },
  {  11, "reservedForUse5" },
  {  12, "reservedForUse6" },
  {  13, "reservedForUse7" },
  {  14, "reservedForUse8" },
  {  15, "eev" },
  { 0, NULL }
};


static int
dissect_dsrc_app_EuroValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     16, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string dsrc_app_CopValue_vals[] = {
  {   0, "noEntry" },
  {   1, "co2class1" },
  {   2, "co2class2" },
  {   3, "co2class3" },
  {   4, "co2class4" },
  {   5, "co2class5" },
  {   6, "co2class6" },
  {   7, "co2class7" },
  {   8, "reservedforUse" },
  { 0, NULL }
};


static int
dissect_dsrc_app_CopValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     9, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t dsrc_app_EnvironmentalCharacteristics_sequence[] = {
  { &hf_dsrc_app_euroValue  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_EuroValue },
  { &hf_dsrc_app_copValue   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_CopValue },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_app_EnvironmentalCharacteristics(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_app_EnvironmentalCharacteristics, dsrc_app_EnvironmentalCharacteristics_sequence);

  return offset;
}


static const value_string dsrc_app_EngineCharacteristics_vals[] = {
  {   0, "noEntry" },
  {   1, "noEngine" },
  {   2, "petrolUnleaded" },
  {   3, "petrolLeaded" },
  {   4, "diesel" },
  {   5, "lPG" },
  {   6, "battery" },
  {   7, "solar" },
  {   8, "hybrid" },
  {   9, "hydrogen" },
  { 0, NULL }
};


static int
dissect_dsrc_app_EngineCharacteristics(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_app_ExhaustEmissionValues_sequence[] = {
  { &hf_dsrc_app_unitType   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_UnitType },
  { &hf_dsrc_app_emissionCO , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_INTEGER_0_32767 },
  { &hf_dsrc_app_emissionHC , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { &hf_dsrc_app_emissionNOX, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { &hf_dsrc_app_emissionHCNOX, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_app_ExhaustEmissionValues(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_app_ExhaustEmissionValues, dsrc_app_ExhaustEmissionValues_sequence);

  return offset;
}



static int
dissect_dsrc_app_Int1(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_app_PassengerCapacity_sequence[] = {
  { &hf_dsrc_app_numberOfSeats, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int1 },
  { &hf_dsrc_app_numberOfStandingPlaces, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int1 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_app_PassengerCapacity(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_app_PassengerCapacity, dsrc_app_PassengerCapacity_sequence);

  return offset;
}


static const per_sequence_t dsrc_app_Provider_sequence[] = {
  { &hf_dsrc_app_countryCode, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_anads_CountryCode },
  { &hf_dsrc_app_providerIdentifier, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_anads_IssuerIdentifier },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_app_Provider(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_app_Provider, dsrc_app_Provider_sequence);

  return offset;
}


static const per_sequence_t dsrc_app_SoundLevel_sequence[] = {
  { &hf_dsrc_app_soundstationary, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int1 },
  { &hf_dsrc_app_sounddriveby, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int1 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_app_SoundLevel(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_app_SoundLevel, dsrc_app_SoundLevel_sequence);

  return offset;
}


static const per_sequence_t dsrc_app_VehicleDimensions_sequence[] = {
  { &hf_dsrc_app_vehicleLengthOverall, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int1 },
  { &hf_dsrc_app_vehicleHeigthOverall, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int1 },
  { &hf_dsrc_app_vehicleWidthOverall, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int1 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_app_VehicleDimensions(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_app_VehicleDimensions, dsrc_app_VehicleDimensions_sequence);

  return offset;
}


static const per_sequence_t dsrc_app_VehicleWeightLimits_sequence[] = {
  { &hf_dsrc_app_vehicleMaxLadenWeight, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { &hf_dsrc_app_vehicleTrainMaximumWeight, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { &hf_dsrc_app_vehicleWeightUnladen, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Int2 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_app_VehicleWeightLimits(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_app_VehicleWeightLimits, dsrc_app_VehicleWeightLimits_sequence);

  return offset;
}


/* --- Module IVI --- --- ---                                                 */

/*--- Cyclic dependencies ---*/

/* ISO14823Code -> ISO14823Attributes -> ISO14823Attributes/_item -> DDD -> DDD/ioList -> DDD-IO -> DDD-IO/dp -> DestinationPlace -> ISO14823Code */
static int dissect_ivi_ISO14823Code(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);




static int
dissect_ivi_IviIdentificationNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 32767U, NULL, TRUE);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_8_OF_IviIdentificationNumber_sequence_of[1] = {
  { &hf_ivi_connectedIviStructures_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_IviIdentificationNumber },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_8_OF_IviIdentificationNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_8_OF_IviIdentificationNumber, ivi_SEQUENCE_SIZE_1_8_OF_IviIdentificationNumber_sequence_of,
                                                  1, 8, FALSE);

  return offset;
}


static const value_string ivi_IviStatus_vals[] = {
  {   0, "new" },
  {   1, "update" },
  {   2, "cancellation" },
  {   3, "negation" },
  { 0, NULL }
};


static int
dissect_ivi_IviStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_IVIManagementContainer_sequence[] = {
  { &hf_ivi_serviceProviderId, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Provider },
  { &hf_ivi_iviIdentificationNumber, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_IviIdentificationNumber },
  { &hf_ivi_timeStamp       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_TimestampIts },
  { &hf_ivi_validFrom       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_TimestampIts },
  { &hf_ivi_validTo         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_TimestampIts },
  { &hf_ivi_connectedIviStructures, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_8_OF_IviIdentificationNumber },
  { &hf_ivi_iviStatus       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_IviStatus },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_IVIManagementContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_IVIManagementContainer, ivi_IVIManagementContainer_sequence);

  return offset;
}



static int
dissect_ivi_Zid(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 32U, NULL, TRUE);

  return offset;
}



static int
dissect_ivi_INTEGER_0_255(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_DeltaPosition_sequence[] = {
  { &hf_ivi_deltaLatitude   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DeltaLatitude },
  { &hf_ivi_deltaLongitude  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DeltaLongitude },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_DeltaPosition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_DeltaPosition, ivi_DeltaPosition_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_32__OF_DeltaPosition_sequence_of[1] = {
  { &hf_ivi_deltaPositions_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_DeltaPosition },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_32__OF_DeltaPosition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_32__OF_DeltaPosition, ivi_SEQUENCE_SIZE_1_32__OF_DeltaPosition_sequence_of,
                                                  1, 32, TRUE);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_32__OF_DeltaReferencePosition_sequence_of[1] = {
  { &hf_ivi_deltaPositionsWithAltitude_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DeltaReferencePosition },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_32__OF_DeltaReferencePosition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_32__OF_DeltaReferencePosition, ivi_SEQUENCE_SIZE_1_32__OF_DeltaReferencePosition_sequence_of,
                                                  1, 32, TRUE);

  return offset;
}


static const per_sequence_t ivi_AbsolutePosition_sequence[] = {
  { &hf_ivi_latitude        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Latitude },
  { &hf_ivi_longitude       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Longitude },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_AbsolutePosition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_AbsolutePosition, ivi_AbsolutePosition_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePosition_sequence_of[1] = {
  { &hf_ivi_absolutePositions_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_AbsolutePosition },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePosition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePosition, ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePosition_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const per_sequence_t ivi_AbsolutePositionWAltitude_sequence[] = {
  { &hf_ivi_latitude        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Latitude },
  { &hf_ivi_longitude       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Longitude },
  { &hf_ivi_altitude        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Altitude },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_AbsolutePositionWAltitude(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_AbsolutePositionWAltitude, ivi_AbsolutePositionWAltitude_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePositionWAltitude_sequence_of[1] = {
  { &hf_ivi_absolutePositionsWithAltitude_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_AbsolutePositionWAltitude },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePositionWAltitude(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePositionWAltitude, ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePositionWAltitude_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const value_string ivi_PolygonalLine_vals[] = {
  {   0, "deltaPositions" },
  {   1, "deltaPositionsWithAltitude" },
  {   2, "absolutePositions" },
  {   3, "absolutePositionsWithAltitude" },
  { 0, NULL }
};

static const per_choice_t ivi_PolygonalLine_choice[] = {
  {   0, &hf_ivi_deltaPositions  , ASN1_EXTENSION_ROOT    , dissect_ivi_SEQUENCE_SIZE_1_32__OF_DeltaPosition },
  {   1, &hf_ivi_deltaPositionsWithAltitude, ASN1_EXTENSION_ROOT    , dissect_ivi_SEQUENCE_SIZE_1_32__OF_DeltaReferencePosition },
  {   2, &hf_ivi_absolutePositions, ASN1_EXTENSION_ROOT    , dissect_ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePosition },
  {   3, &hf_ivi_absolutePositionsWithAltitude, ASN1_EXTENSION_ROOT    , dissect_ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePositionWAltitude },
  { 0, NULL, 0, NULL }
};

static int
dissect_ivi_PolygonalLine(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_ivi_PolygonalLine, ivi_PolygonalLine_choice,
                                 NULL);

  return offset;
}



static int
dissect_ivi_IVILaneWidth(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 1023U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_Segment_sequence[] = {
  { &hf_ivi_line            , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_PolygonalLine },
  { &hf_ivi_laneWidth       , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_IVILaneWidth },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_Segment(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_Segment, ivi_Segment_sequence);

  return offset;
}



static int
dissect_ivi_INTEGER_M32768_32767(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -32768, 32767U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_ComputedSegment_sequence[] = {
  { &hf_ivi_zoneId          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_Zid },
  { &hf_ivi_laneNumber      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_LanePosition },
  { &hf_ivi_laneWidth       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_IVILaneWidth },
  { &hf_ivi_offsetDistance  , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_M32768_32767 },
  { &hf_ivi_offsetPosition  , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_DeltaReferencePosition },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_ComputedSegment(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_ComputedSegment, ivi_ComputedSegment_sequence);

  return offset;
}


static const value_string ivi_Zone_vals[] = {
  {   0, "segment" },
  {   1, "area" },
  {   2, "computedSegment" },
  { 0, NULL }
};

static const per_choice_t ivi_Zone_choice[] = {
  {   0, &hf_ivi_segment         , ASN1_EXTENSION_ROOT    , dissect_ivi_Segment },
  {   1, &hf_ivi_area            , ASN1_EXTENSION_ROOT    , dissect_ivi_PolygonalLine },
  {   2, &hf_ivi_computedSegment , ASN1_EXTENSION_ROOT    , dissect_ivi_ComputedSegment },
  { 0, NULL, 0, NULL }
};

static int
dissect_ivi_Zone(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_ivi_Zone, ivi_Zone_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t ivi_GlcPart_sequence[] = {
  { &hf_ivi_zoneId          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_Zid },
  { &hf_ivi_laneNumber      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_LanePosition },
  { &hf_ivi_zoneExtension   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_INTEGER_0_255 },
  { &hf_ivi_zoneHeading     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_HeadingValue },
  { &hf_ivi_zone            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_Zone },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_GlcPart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_GlcPart, ivi_GlcPart_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_16__OF_GlcPart_sequence_of[1] = {
  { &hf_ivi_parts_item      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_GlcPart },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_16__OF_GlcPart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_16__OF_GlcPart, ivi_SEQUENCE_SIZE_1_16__OF_GlcPart_sequence_of,
                                                  1, 16, TRUE);

  return offset;
}


static const per_sequence_t ivi_GeographicLocationContainer_sequence[] = {
  { &hf_ivi_referencePosition, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_ReferencePosition },
  { &hf_ivi_referencePositionTime, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_TimestampIts },
  { &hf_ivi_referencePositionHeading, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_Heading },
  { &hf_ivi_referencePositionSpeed, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_Speed },
  { &hf_ivi_parts           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_SEQUENCE_SIZE_1_16__OF_GlcPart },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_GeographicLocationContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_GeographicLocationContainer, ivi_GeographicLocationContainer_sequence);

  return offset;
}


static const per_sequence_t ivi_T_GicPartDetectionZoneIds_sequence_of[1] = {
  { &hf_ivi_gpDetectionZoneIds_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_Zid },
};

static int
dissect_ivi_T_GicPartDetectionZoneIds(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_GicPartDetectionZoneIds, ivi_T_GicPartDetectionZoneIds_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const per_sequence_t ivi_T_GicPartRelevanceZoneIds_sequence_of[1] = {
  { &hf_ivi_gpRelevanceZoneIds_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_Zid },
};

static int
dissect_ivi_T_GicPartRelevanceZoneIds(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_GicPartRelevanceZoneIds, ivi_T_GicPartRelevanceZoneIds_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const value_string ivi_Direction_vals[] = {
  {   0, "sameDirection" },
  {   1, "oppositeDirection" },
  {   2, "bothDirections" },
  {   3, "valueNotUsed" },
  { 0, NULL }
};


static int
dissect_ivi_Direction(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 3U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_T_GicPartDriverAwarenessZoneIds_sequence_of[1] = {
  { &hf_ivi_gpDriverAwarenessZoneIds_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_Zid },
};

static int
dissect_ivi_T_GicPartDriverAwarenessZoneIds(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_GicPartDriverAwarenessZoneIds, ivi_T_GicPartDriverAwarenessZoneIds_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_8__OF_LanePosition_sequence_of[1] = {
  { &hf_ivi_applicableLanes_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_LanePosition },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_8__OF_LanePosition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_8__OF_LanePosition, ivi_SEQUENCE_SIZE_1_8__OF_LanePosition_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const value_string ivi_IviType_vals[] = {
  {   0, "immediateDangerWarningMessages" },
  {   1, "regulatoryMessages" },
  {   2, "trafficRelatedInformationMessages" },
  {   3, "pollutionMessages" },
  {   4, "notTrafficRelatedInformationMessages" },
  { 0, NULL }
};


static int
dissect_ivi_IviType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}


static const value_string ivi_IviPurpose_vals[] = {
  {   0, "safety" },
  {   1, "environmental" },
  {   2, "trafficOptimisation" },
  { 0, NULL }
};


static int
dissect_ivi_IviPurpose(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 3U, NULL, FALSE);

  return offset;
}


static const value_string ivi_LaneStatus_vals[] = {
  {   0, "open" },
  {   1, "closed" },
  {   2, "mergeR" },
  {   3, "mergeL" },
  {   4, "mergeLR" },
  {   5, "provisionallyOpen" },
  {   6, "diverging" },
  { 0, NULL }
};


static int
dissect_ivi_LaneStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, TRUE);

  return offset;
}


static const value_string ivi_GoodsType_vals[] = {
  {   0, "ammunition" },
  {   1, "chemicals" },
  {   2, "empty" },
  {   3, "fuel" },
  {   4, "glass" },
  {   5, "dangerous" },
  {   6, "liquid" },
  {   7, "liveStock" },
  {   8, "dangerousForPeople" },
  {   9, "dangerousForTheEnvironment" },
  {  10, "dangerousForWater" },
  {  11, "perishableProducts" },
  {  12, "pharmaceutical" },
  {  13, "vehicles" },
  { 0, NULL }
};


static int
dissect_ivi_GoodsType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 15U, NULL, TRUE);

  return offset;
}


static const per_sequence_t ivi_LoadType_sequence[] = {
  { &hf_ivi_goodsType       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_GoodsType },
  { &hf_ivi_dangerousGoodsType, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DangerousGoodsBasic },
  { &hf_ivi_specialTransportType, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_SpecialTransportType },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_LoadType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_LoadType, ivi_LoadType_sequence);

  return offset;
}


static const value_string ivi_VehicleCharacteristicsFixValues_vals[] = {
  {   0, "simpleVehicleType" },
  {   1, "euVehicleCategoryCode" },
  {   2, "iso3833VehicleType" },
  {   3, "euroAndCo2value" },
  {   4, "engineCharacteristics" },
  {   5, "loadType" },
  {   6, "usage" },
  { 0, NULL }
};

static const per_choice_t ivi_VehicleCharacteristicsFixValues_choice[] = {
  {   0, &hf_ivi_simpleVehicleType, ASN1_EXTENSION_ROOT    , dissect_its_StationType },
  {   1, &hf_ivi_euVehicleCategoryCode, ASN1_EXTENSION_ROOT    , dissect_erivdm_EuVehicleCategoryCode },
  {   2, &hf_ivi_iso3833VehicleType, ASN1_EXTENSION_ROOT    , dissect_erivdm_Iso3833VehicleType },
  {   3, &hf_ivi_euroAndCo2value , ASN1_EXTENSION_ROOT    , dissect_dsrc_app_EnvironmentalCharacteristics },
  {   4, &hf_ivi_engineCharacteristics, ASN1_EXTENSION_ROOT    , dissect_dsrc_app_EngineCharacteristics },
  {   5, &hf_ivi_loadType        , ASN1_EXTENSION_ROOT    , dissect_ivi_LoadType },
  {   6, &hf_ivi_usage           , ASN1_EXTENSION_ROOT    , dissect_its_VehicleRole },
  { 0, NULL, 0, NULL }
};

static int
dissect_ivi_VehicleCharacteristicsFixValues(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_ivi_VehicleCharacteristicsFixValues, ivi_VehicleCharacteristicsFixValues_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t ivi_T_TractorCharactEqualTo_sequence_of[1] = {
  { &hf_ivi_toEqualTo_item  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_VehicleCharacteristicsFixValues },
};

static int
dissect_ivi_T_TractorCharactEqualTo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_TractorCharactEqualTo, ivi_T_TractorCharactEqualTo_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}


static const per_sequence_t ivi_T_TractorCharactNotEqualTo_sequence_of[1] = {
  { &hf_ivi_toNotEqualTo_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_VehicleCharacteristicsFixValues },
};

static int
dissect_ivi_T_TractorCharactNotEqualTo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_TractorCharactNotEqualTo, ivi_T_TractorCharactNotEqualTo_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}


static const value_string ivi_ComparisonOperator_vals[] = {
  {   0, "greaterThan" },
  {   1, "greaterThanOrEqualTo" },
  {   2, "lessThan" },
  {   3, "lessThanOrEqualTo" },
  { 0, NULL }
};


static int
dissect_ivi_ComparisonOperator(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 3U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_INTEGER_0_7(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}


static const value_string ivi_T_limits_vals[] = {
  {   0, "numberOfAxles" },
  {   1, "vehicleDimensions" },
  {   2, "vehicleWeightLimits" },
  {   3, "axleWeightLimits" },
  {   4, "passengerCapacity" },
  {   5, "exhaustEmissionValues" },
  {   6, "dieselEmissionValues" },
  {   7, "soundLevel" },
  { 0, NULL }
};

static const per_choice_t ivi_T_limits_choice[] = {
  {   0, &hf_ivi_numberOfAxles   , ASN1_EXTENSION_ROOT    , dissect_ivi_INTEGER_0_7 },
  {   1, &hf_ivi_vehicleDimensions, ASN1_EXTENSION_ROOT    , dissect_dsrc_app_VehicleDimensions },
  {   2, &hf_ivi_vehicleWeightLimits, ASN1_EXTENSION_ROOT    , dissect_dsrc_app_VehicleWeightLimits },
  {   3, &hf_ivi_axleWeightLimits, ASN1_EXTENSION_ROOT    , dissect_dsrc_app_AxleWeightLimits },
  {   4, &hf_ivi_passengerCapacity, ASN1_EXTENSION_ROOT    , dissect_dsrc_app_PassengerCapacity },
  {   5, &hf_ivi_exhaustEmissionValues, ASN1_EXTENSION_ROOT    , dissect_dsrc_app_ExhaustEmissionValues },
  {   6, &hf_ivi_dieselEmissionValues, ASN1_EXTENSION_ROOT    , dissect_dsrc_app_DieselEmissionValues },
  {   7, &hf_ivi_soundLevel      , ASN1_EXTENSION_ROOT    , dissect_dsrc_app_SoundLevel },
  { 0, NULL, 0, NULL }
};

static int
dissect_ivi_T_limits(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_ivi_T_limits, ivi_T_limits_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t ivi_VehicleCharacteristicsRanges_sequence[] = {
  { &hf_ivi_comparisonOperator, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_ComparisonOperator },
  { &hf_ivi_limits          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_T_limits },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_VehicleCharacteristicsRanges(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_VehicleCharacteristicsRanges, ivi_VehicleCharacteristicsRanges_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_4__OF_VehicleCharacteristicsRanges_sequence_of[1] = {
  { &hf_ivi_ranges_item     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_VehicleCharacteristicsRanges },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_4__OF_VehicleCharacteristicsRanges(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_4__OF_VehicleCharacteristicsRanges, ivi_SEQUENCE_SIZE_1_4__OF_VehicleCharacteristicsRanges_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}


static const per_sequence_t ivi_TractorCharacteristics_sequence[] = {
  { &hf_ivi_toEqualTo       , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_T_TractorCharactEqualTo },
  { &hf_ivi_toNotEqualTo    , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_T_TractorCharactNotEqualTo },
  { &hf_ivi_ranges          , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_4__OF_VehicleCharacteristicsRanges },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_TractorCharacteristics(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_TractorCharacteristics, ivi_TractorCharacteristics_sequence);

  return offset;
}


static const per_sequence_t ivi_T_TrailerCharactEqualTo_sequence_of[1] = {
  { &hf_ivi_teEqualTo_item  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_VehicleCharacteristicsFixValues },
};

static int
dissect_ivi_T_TrailerCharactEqualTo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_TrailerCharactEqualTo, ivi_T_TrailerCharactEqualTo_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}


static const per_sequence_t ivi_T_TrailerCharactNotEqualTo_sequence_of[1] = {
  { &hf_ivi_teNotEqualTo_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_VehicleCharacteristicsFixValues },
};

static int
dissect_ivi_T_TrailerCharactNotEqualTo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_TrailerCharactNotEqualTo, ivi_T_TrailerCharactNotEqualTo_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}


static const per_sequence_t ivi_TrailerCharacteristics_sequence[] = {
  { &hf_ivi_teEqualTo       , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_T_TrailerCharactEqualTo },
  { &hf_ivi_teNotEqualTo    , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_T_TrailerCharactNotEqualTo },
  { &hf_ivi_ranges          , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_4__OF_VehicleCharacteristicsRanges },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_TrailerCharacteristics(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_TrailerCharacteristics, ivi_TrailerCharacteristics_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_3_OF_TrailerCharacteristics_sequence_of[1] = {
  { &hf_ivi_trailer_item    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_TrailerCharacteristics },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_3_OF_TrailerCharacteristics(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_3_OF_TrailerCharacteristics, ivi_SEQUENCE_SIZE_1_3_OF_TrailerCharacteristics_sequence_of,
                                                  1, 3, FALSE);

  return offset;
}



static int
dissect_ivi_TrainCharacteristics(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ivi_TractorCharacteristics(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const per_sequence_t ivi_CompleteVehicleCharacteristics_sequence[] = {
  { &hf_ivi_tractor         , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_TractorCharacteristics },
  { &hf_ivi_trailer         , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_3_OF_TrailerCharacteristics },
  { &hf_ivi_train           , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_TrainCharacteristics },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_CompleteVehicleCharacteristics(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_CompleteVehicleCharacteristics, ivi_CompleteVehicleCharacteristics_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_8__OF_CompleteVehicleCharacteristics_sequence_of[1] = {
  { &hf_ivi_vehicleCharacteristics_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_CompleteVehicleCharacteristics },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_8__OF_CompleteVehicleCharacteristics(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_8__OF_CompleteVehicleCharacteristics, ivi_SEQUENCE_SIZE_1_8__OF_CompleteVehicleCharacteristics_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const value_string ivi_DriverCharacteristics_vals[] = {
  {   0, "unexperiencedDrivers" },
  {   1, "experiencedDrivers" },
  {   2, "rfu1" },
  {   3, "rfu2" },
  { 0, NULL }
};


static int
dissect_ivi_DriverCharacteristics(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 3U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_INTEGER_1_4_(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 4U, NULL, TRUE);

  return offset;
}



static int
dissect_ivi_INTEGER_1_64_(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 64U, NULL, TRUE);

  return offset;
}


static const value_string ivi_VcClass_vals[] = {
  {   0, "classA" },
  {   1, "classB" },
  {   2, "classC" },
  {   3, "classD" },
  {   4, "classE" },
  {   5, "classF" },
  {   6, "classG" },
  {   7, "classH" },
  { 0, NULL }
};


static int
dissect_ivi_VcClass(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_INTEGER_1_64(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 64U, NULL, FALSE);

  return offset;
}


static const value_string ivi_VcOption_vals[] = {
  {   0, "none" },
  {   1, "a" },
  {   2, "b" },
  {   3, "c" },
  {   4, "d" },
  {   5, "e" },
  {   6, "f" },
  {   7, "g" },
  { 0, NULL }
};


static int
dissect_ivi_VcOption(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_INTEGER_2000_2127_(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            2000U, 2127U, NULL, TRUE);

  return offset;
}


static const per_sequence_t ivi_T_year_sequence[] = {
  { &hf_ivi_syr             , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_2000_2127_ },
  { &hf_ivi_eyr             , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_2000_2127_ },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_T_year(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_T_year, ivi_T_year_sequence);

  return offset;
}



static int
dissect_ivi_INTEGER_1_12(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 12U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_INTEGER_1_31(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 31U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_MonthDay_sequence[] = {
  { &hf_ivi_month           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_1_12 },
  { &hf_ivi_day             , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_1_31 },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_MonthDay(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_MonthDay, ivi_MonthDay_sequence);

  return offset;
}


static const per_sequence_t ivi_T_month_day_sequence[] = {
  { &hf_ivi_smd             , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_MonthDay },
  { &hf_ivi_emd             , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_MonthDay },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_T_month_day(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_T_month_day, ivi_T_month_day_sequence);

  return offset;
}



static int
dissect_ivi_PMD(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 651 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     4, 4, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = {
      &hf_ivi_PMD_national_holiday,
      &hf_ivi_PMD_even_days,
      &hf_ivi_PMD_odd_days,
      &hf_ivi_PMD_market_day,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_ivi_PMD);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 2, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_ivi_INTEGER_0_23(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 23U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_INTEGER_0_59(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 59U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_HoursMinutes_sequence[] = {
  { &hf_ivi_hours           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_0_23 },
  { &hf_ivi_mins            , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_0_59 },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_HoursMinutes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_HoursMinutes, ivi_HoursMinutes_sequence);

  return offset;
}


static const per_sequence_t ivi_T_hourMinutes_sequence[] = {
  { &hf_ivi_shm             , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_HoursMinutes },
  { &hf_ivi_ehm             , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_HoursMinutes },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_T_hourMinutes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_T_hourMinutes, ivi_T_hourMinutes_sequence);

  return offset;
}



static int
dissect_ivi_DayOfWeek(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 787 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     8, 8, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_ivi_DayOfWeek_unused,
      &hf_ivi_DayOfWeek_monday,
      &hf_ivi_DayOfWeek_tuesday,
      &hf_ivi_DayOfWeek_wednesday,
      &hf_ivi_DayOfWeek_thursday,
      &hf_ivi_DayOfWeek_friday,
      &hf_ivi_DayOfWeek_saturday,
      &hf_ivi_DayOfWeek_sunday,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_ivi_DayOfWeek);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const per_sequence_t ivi_DTM_sequence[] = {
  { &hf_ivi_year            , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_T_year },
  { &hf_ivi_month_day       , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_T_month_day },
  { &hf_ivi_pmd             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_PMD },
  { &hf_ivi_hourMinutes     , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_T_hourMinutes },
  { &hf_ivi_dayOfWeek       , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_DayOfWeek },
  { &hf_ivi_period          , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_HoursMinutes },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_DTM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_DTM, ivi_DTM_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_8__OF_DTM_sequence_of[1] = {
  { &hf_ivi_vcValidity_item , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_DTM },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_8__OF_DTM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_8__OF_DTM, ivi_SEQUENCE_SIZE_1_8__OF_DTM_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}



static int
dissect_ivi_INTEGER_0_65535(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const value_string ivi_RSCUnit_vals[] = {
  {   0, "kmperh" },
  {   1, "milesperh" },
  {   2, "kilometer" },
  {   3, "meter" },
  {   4, "decimeter" },
  {   5, "centimeter" },
  {   6, "mile" },
  {   7, "yard" },
  {   8, "foot" },
  {   9, "minutesOfTime" },
  {  10, "tonnes" },
  {  11, "hundredkg" },
  {  12, "pound" },
  {  13, "rateOfIncline" },
  { 0, NULL }
};


static int
dissect_ivi_RSCUnit(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 15U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_VcCode_sequence[] = {
  { &hf_ivi_roadSignClass   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_VcClass },
  { &hf_ivi_roadSignCode    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_1_64 },
  { &hf_ivi_vcOption        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_VcOption },
  { &hf_ivi_vcValidity      , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_8__OF_DTM },
  { &hf_ivi_vcValue         , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_0_65535 },
  { &hf_ivi_unit            , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_RSCUnit },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_VcCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_VcCode, ivi_VcCode_sequence);

  return offset;
}



static int
dissect_ivi_OCTET_STRING_SIZE_2(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       2, 2, FALSE, NULL);

  return offset;
}


static const value_string ivi_T_trafficSignPictogram_vals[] = {
  {   0, "dangerWarning" },
  {   1, "regulatory" },
  {   2, "informative" },
  { 0, NULL }
};


static int
dissect_ivi_T_trafficSignPictogram(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string ivi_T_publicFacilitiesPictogram_vals[] = {
  {   0, "publicFacilities" },
  { 0, NULL }
};


static int
dissect_ivi_T_publicFacilitiesPictogram(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     1, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string ivi_T_ambientOrRoadConditionPictogram_vals[] = {
  {   0, "ambientCondition" },
  {   1, "roadCondition" },
  { 0, NULL }
};


static int
dissect_ivi_T_ambientOrRoadConditionPictogram(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string ivi_T_serviceCategoryCode_vals[] = {
  {   0, "trafficSignPictogram" },
  {   1, "publicFacilitiesPictogram" },
  {   2, "ambientOrRoadConditionPictogram" },
  { 0, NULL }
};

static const per_choice_t ivi_T_serviceCategoryCode_choice[] = {
  {   0, &hf_ivi_trafficSignPictogram, ASN1_EXTENSION_ROOT    , dissect_ivi_T_trafficSignPictogram },
  {   1, &hf_ivi_publicFacilitiesPictogram, ASN1_EXTENSION_ROOT    , dissect_ivi_T_publicFacilitiesPictogram },
  {   2, &hf_ivi_ambientOrRoadConditionPictogram, ASN1_EXTENSION_ROOT    , dissect_ivi_T_ambientOrRoadConditionPictogram },
  { 0, NULL, 0, NULL }
};

static int
dissect_ivi_T_serviceCategoryCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_ivi_T_serviceCategoryCode, ivi_T_serviceCategoryCode_choice,
                                 NULL);

  return offset;
}



static int
dissect_ivi_INTEGER_1_9(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 9U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_INTEGER_0_99(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 99U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_T_pictogramCategoryCode_sequence[] = {
  { &hf_ivi_nature          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_1_9 },
  { &hf_ivi_serialNumber    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_0_99 },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_T_pictogramCategoryCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_T_pictogramCategoryCode, ivi_T_pictogramCategoryCode_sequence);

  return offset;
}


static const per_sequence_t ivi_T_icPictogramCode_sequence[] = {
  { &hf_ivi_countryCode     , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_OCTET_STRING_SIZE_2 },
  { &hf_ivi_serviceCategoryCode, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_T_serviceCategoryCode },
  { &hf_ivi_pictogramCategoryCode, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_T_pictogramCategoryCode },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_T_icPictogramCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_T_icPictogramCode, ivi_T_icPictogramCode_sequence);

  return offset;
}



static int
dissect_ivi_EDT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ivi_DTM(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const value_string ivi_DFL_vals[] = {
  {   1, "sDL" },
  {   2, "sLT" },
  {   3, "sRT" },
  {   4, "lTO" },
  {   5, "rTO" },
  {   6, "cLL" },
  {   7, "cRI" },
  {   8, "oVL" },
  { 0, NULL }
};


static int
dissect_ivi_DFL(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 8U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_INTEGER_1_16384(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 16384U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_Distance_sequence[] = {
  { &hf_ivi_dValue          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_1_16384 },
  { &hf_ivi_unit            , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_RSCUnit },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_Distance(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_Distance, ivi_Distance_sequence);

  return offset;
}


static const per_sequence_t ivi_Weight_sequence[] = {
  { &hf_ivi_wValue          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_1_16384 },
  { &hf_ivi_unit            , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_RSCUnit },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_Weight(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_Weight, ivi_Weight_sequence);

  return offset;
}


static const per_sequence_t ivi_VED_sequence[] = {
  { &hf_ivi_hei             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_Distance },
  { &hf_ivi_wid             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_Distance },
  { &hf_ivi_vln             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_Distance },
  { &hf_ivi_wei             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_Weight },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_VED(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_VED, ivi_VED_sequence);

  return offset;
}



static int
dissect_ivi_INTEGER_0_250(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 250U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_SPE_sequence[] = {
  { &hf_ivi_spm             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_0_250 },
  { &hf_ivi_mns             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_0_250 },
  { &hf_ivi_unit            , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_RSCUnit },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_SPE(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_SPE, ivi_SPE_sequence);

  return offset;
}



static int
dissect_ivi_ROI(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 32U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_DBV(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ivi_Distance(tvb, offset, actx, tree, hf_index);

  return offset;
}



static int
dissect_ivi_INTEGER_1_128(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 128U, NULL, FALSE);

  return offset;
}


static const value_string ivi_DDD_DEP_vals[] = {
  {   0, "none" },
  {   1, "importantArea" },
  {   2, "principalArea" },
  {   3, "generalArea" },
  {   4, "wellKnownPoint" },
  {   5, "country" },
  {   6, "city" },
  {   7, "street" },
  {   8, "industrialArea" },
  {   9, "historicArea" },
  {  10, "touristicArea" },
  {  11, "culturalArea" },
  {  12, "touristicRoute" },
  {  13, "recommendedRoute" },
  {  14, "touristicAttraction" },
  {  15, "geographicArea" },
  { 0, NULL }
};


static int
dissect_ivi_DDD_DEP(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 15U, NULL, TRUE);

  return offset;
}



static int
dissect_ivi_OCTET_STRING(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       NO_BOUND, NO_BOUND, FALSE, NULL);

  return offset;
}



static int
dissect_ivi_INTEGER_1_999(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 999U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_UTF8String(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          NO_BOUND, NO_BOUND, FALSE);

  return offset;
}


static const per_sequence_t ivi_DestinationPlace_sequence[] = {
  { &hf_ivi_depType         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_DDD_DEP },
  { &hf_ivi_depRSCode       , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_ISO14823Code },
  { &hf_ivi_depBlob         , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_OCTET_STRING },
  { &hf_ivi_plnId           , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_999 },
  { &hf_ivi_plnText         , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_UTF8String },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_DestinationPlace(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_DestinationPlace, ivi_DestinationPlace_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_4__OF_DestinationPlace_sequence_of[1] = {
  { &hf_ivi_dp_item         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_DestinationPlace },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_4__OF_DestinationPlace(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_4__OF_DestinationPlace, ivi_SEQUENCE_SIZE_1_4__OF_DestinationPlace_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}


static const value_string ivi_DDD_DER_vals[] = {
  {   0, "none" },
  {   1, "nationalHighway" },
  {   2, "localHighway" },
  {   3, "tollExpresswayMotorway" },
  {   4, "internationalHighway" },
  {   5, "highway" },
  {   6, "expressway" },
  {   7, "nationalRoad" },
  {   8, "regionalProvincialRoad" },
  {   9, "localRoad" },
  {  10, "motorwayJunction" },
  {  11, "diversion" },
  {  12, "rfu1" },
  {  13, "rfu2" },
  {  14, "rfu3" },
  {  15, "rfu4" },
  { 0, NULL }
};


static int
dissect_ivi_DDD_DER(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 15U, NULL, TRUE);

  return offset;
}


static const per_sequence_t ivi_DestinationRoad_sequence[] = {
  { &hf_ivi_derType         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_DDD_DER },
  { &hf_ivi_ronId           , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_999 },
  { &hf_ivi_ronText         , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_UTF8String },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_DestinationRoad(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_DestinationRoad, ivi_DestinationRoad_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_4__OF_DestinationRoad_sequence_of[1] = {
  { &hf_ivi_dr_item         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_DestinationRoad },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_4__OF_DestinationRoad(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_4__OF_DestinationRoad, ivi_SEQUENCE_SIZE_1_4__OF_DestinationRoad_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}


static const per_sequence_t ivi_DistanceOrDuration_sequence[] = {
  { &hf_ivi_dodValue        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_1_16384 },
  { &hf_ivi_unit            , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_RSCUnit },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_DistanceOrDuration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_DistanceOrDuration, ivi_DistanceOrDuration_sequence);

  return offset;
}


static const per_sequence_t ivi_DDD_IO_sequence[] = {
  { &hf_ivi_drn             , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_0_7 },
  { &hf_ivi_dp              , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_4__OF_DestinationPlace },
  { &hf_ivi_dr              , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_4__OF_DestinationRoad },
  { &hf_ivi_rne             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_999 },
  { &hf_ivi_stnId           , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_999 },
  { &hf_ivi_stnText         , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_UTF8String },
  { &hf_ivi_dcp             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_DistanceOrDuration },
  { &hf_ivi_ddp             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_DistanceOrDuration },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_DDD_IO(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_DDD_IO, ivi_DDD_IO_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_8__OF_DDD_IO_sequence_of[1] = {
  { &hf_ivi_ioList_item     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_DDD_IO },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_8__OF_DDD_IO(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_8__OF_DDD_IO, ivi_SEQUENCE_SIZE_1_8__OF_DDD_IO_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const per_sequence_t ivi_DDD_sequence[] = {
  { &hf_ivi_dcj             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_128 },
  { &hf_ivi_dcr             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_128 },
  { &hf_ivi_tpl             , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_128 },
  { &hf_ivi_ioList          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_SEQUENCE_SIZE_1_8__OF_DDD_IO },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_DDD(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_DDD, ivi_DDD_sequence);

  return offset;
}


static const value_string ivi_ISO14823Attributes_item_vals[] = {
  {   0, "dtm" },
  {   1, "edt" },
  {   2, "dfl" },
  {   3, "ved" },
  {   4, "spe" },
  {   5, "roi" },
  {   6, "dbv" },
  {   7, "ddd" },
  { 0, NULL }
};

static const per_choice_t ivi_ISO14823Attributes_item_choice[] = {
  {   0, &hf_ivi_dtm             , ASN1_NO_EXTENSIONS     , dissect_ivi_DTM },
  {   1, &hf_ivi_edt             , ASN1_NO_EXTENSIONS     , dissect_ivi_EDT },
  {   2, &hf_ivi_dfl             , ASN1_NO_EXTENSIONS     , dissect_ivi_DFL },
  {   3, &hf_ivi_ved             , ASN1_NO_EXTENSIONS     , dissect_ivi_VED },
  {   4, &hf_ivi_spe             , ASN1_NO_EXTENSIONS     , dissect_ivi_SPE },
  {   5, &hf_ivi_roi             , ASN1_NO_EXTENSIONS     , dissect_ivi_ROI },
  {   6, &hf_ivi_dbv             , ASN1_NO_EXTENSIONS     , dissect_ivi_DBV },
  {   7, &hf_ivi_ddd             , ASN1_NO_EXTENSIONS     , dissect_ivi_DDD },
  { 0, NULL, 0, NULL }
};

static int
dissect_ivi_ISO14823Attributes_item(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_ivi_ISO14823Attributes_item, ivi_ISO14823Attributes_item_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t ivi_ISO14823Attributes_sequence_of[1] = {
  { &hf_ivi_ISO14823Attributes_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_ISO14823Attributes_item },
};

static int
dissect_ivi_ISO14823Attributes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_ISO14823Attributes, ivi_ISO14823Attributes_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const per_sequence_t ivi_ISO14823Code_sequence[] = {
  { &hf_ivi_icPictogramCode , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_T_icPictogramCode },
  { &hf_ivi_attributes      , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_ISO14823Attributes },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_ISO14823Code(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_ISO14823Code, ivi_ISO14823Code_sequence);

  return offset;
}


static const per_sequence_t ivi_AnyCatalogue_sequence[] = {
  { &hf_ivi_owner           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_app_Provider },
  { &hf_ivi_version         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_0_255 },
  { &hf_ivi_acPictogramCode , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_0_65535 },
  { &hf_ivi_acValue         , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_0_65535 },
  { &hf_ivi_unit            , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_RSCUnit },
  { &hf_ivi_attributes      , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_ISO14823Attributes },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_AnyCatalogue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_AnyCatalogue, ivi_AnyCatalogue_sequence);

  return offset;
}


static const value_string ivi_T_code_vals[] = {
  {   0, "viennaConvention" },
  {   1, "iso14823" },
  {   2, "itisCodes" },
  {   3, "anyCatalogue" },
  { 0, NULL }
};

static const per_choice_t ivi_T_code_choice[] = {
  {   0, &hf_ivi_viennaConvention, ASN1_EXTENSION_ROOT    , dissect_ivi_VcCode },
  {   1, &hf_ivi_iso14823        , ASN1_EXTENSION_ROOT    , dissect_ivi_ISO14823Code },
  {   2, &hf_ivi_itisCodes       , ASN1_EXTENSION_ROOT    , dissect_ivi_INTEGER_0_65535 },
  {   3, &hf_ivi_anyCatalogue    , ASN1_EXTENSION_ROOT    , dissect_ivi_AnyCatalogue },
  { 0, NULL, 0, NULL }
};

static int
dissect_ivi_T_code(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_ivi_T_code, ivi_T_code_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t ivi_RSCode_sequence[] = {
  { &hf_ivi_rscLayoutComponentId, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_4_ },
  { &hf_ivi_code            , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_T_code },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_RSCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_RSCode, ivi_RSCode_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_4__OF_RSCode_sequence_of[1] = {
  { &hf_ivi_roadSignCodes_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_RSCode },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_4__OF_RSCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_4__OF_RSCode, ivi_SEQUENCE_SIZE_1_4__OF_RSCode_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}



static int
dissect_ivi_BIT_STRING_SIZE_10(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     10, 10, FALSE, NULL, NULL);

  return offset;
}


static const per_sequence_t ivi_Text_sequence[] = {
  { &hf_ivi_tLayoutComponentId, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_4_ },
  { &hf_ivi_language        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_BIT_STRING_SIZE_10 },
  { &hf_ivi_textContent     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_UTF8String },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_Text(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_Text, ivi_Text_sequence);

  return offset;
}


static const per_sequence_t ivi_T_GicPartExtraText_sequence_of[1] = {
  { &hf_ivi_extraText_item  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_Text },
};

static int
dissect_ivi_T_GicPartExtraText(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_GicPartExtraText, ivi_T_GicPartExtraText_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}


static const per_sequence_t ivi_GicPart_sequence[] = {
  { &hf_ivi_gpDetectionZoneIds, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_T_GicPartDetectionZoneIds },
  { &hf_ivi_its_Rrid        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_csmid_VarLengthNumber },
  { &hf_ivi_gpRelevanceZoneIds, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_T_GicPartRelevanceZoneIds },
  { &hf_ivi_direction       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_Direction },
  { &hf_ivi_gpDriverAwarenessZoneIds, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_T_GicPartDriverAwarenessZoneIds },
  { &hf_ivi_minimumAwarenessTime, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_INTEGER_0_255 },
  { &hf_ivi_applicableLanes , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_8__OF_LanePosition },
  { &hf_ivi_iviType         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_IviType },
  { &hf_ivi_iviPurpose      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_IviPurpose },
  { &hf_ivi_laneStatus      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_LaneStatus },
  { &hf_ivi_vehicleCharacteristics, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_8__OF_CompleteVehicleCharacteristics },
  { &hf_ivi_driverCharacteristics, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_DriverCharacteristics },
  { &hf_ivi_layoutId        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_4_ },
  { &hf_ivi_preStoredlayoutId, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_64_ },
  { &hf_ivi_roadSignCodes   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_SEQUENCE_SIZE_1_4__OF_RSCode },
  { &hf_ivi_extraText       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_T_GicPartExtraText },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_GicPart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_GicPart, ivi_GicPart_sequence);

  return offset;
}


static const per_sequence_t ivi_GeneralIviContainer_sequence_of[1] = {
  { &hf_ivi_GeneralIviContainer_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_GicPart },
};

static int
dissect_ivi_GeneralIviContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_GeneralIviContainer, ivi_GeneralIviContainer_sequence_of,
                                                  1, 16, TRUE);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_8__OF_Zid_sequence_of[1] = {
  { &hf_ivi_zoneIds_item    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_Zid },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_8__OF_Zid(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_8__OF_Zid, ivi_SEQUENCE_SIZE_1_8__OF_Zid_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const value_string ivi_LaneType_vals[] = {
  {   0, "traffic" },
  {   1, "through" },
  {   2, "reversible" },
  {   3, "acceleration" },
  {   4, "deceleration" },
  {   5, "leftHandTurning" },
  {   6, "rightHandTurning" },
  {   7, "dedicatedVehicle" },
  {   8, "bus" },
  {   9, "taxi" },
  {  10, "hov" },
  {  11, "hot" },
  {  12, "pedestrian" },
  {  13, "bikeLane" },
  {  14, "median" },
  {  15, "striping" },
  {  16, "trackedVehicle" },
  {  17, "parking" },
  {  18, "emergency" },
  {  19, "verge" },
  { 0, NULL }
};


static int
dissect_ivi_LaneType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 31U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_LaneInformation_sequence[] = {
  { &hf_ivi_laneNumber      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_LanePosition },
  { &hf_ivi_direction       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_Direction },
  { &hf_ivi_liValidity      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_DTM },
  { &hf_ivi_laneType        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_LaneType },
  { &hf_ivi_laneTypeQualifier, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_CompleteVehicleCharacteristics },
  { &hf_ivi_laneStatus      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_LaneStatus },
  { &hf_ivi_laneWidth       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_IVILaneWidth },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_LaneInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_LaneInformation, ivi_LaneInformation_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_16__OF_LaneInformation_sequence_of[1] = {
  { &hf_ivi_laneConfiguration_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_LaneInformation },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_16__OF_LaneInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_16__OF_LaneInformation, ivi_SEQUENCE_SIZE_1_16__OF_LaneInformation_sequence_of,
                                                  1, 16, TRUE);

  return offset;
}


static const per_sequence_t ivi_RccPart_sequence[] = {
  { &hf_ivi_zoneIds         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_SEQUENCE_SIZE_1_8__OF_Zid },
  { &hf_ivi_roadType        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_RoadType },
  { &hf_ivi_laneConfiguration, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_SEQUENCE_SIZE_1_16__OF_LaneInformation },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_RccPart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_RccPart, ivi_RccPart_sequence);

  return offset;
}


static const per_sequence_t ivi_RoadConfigurationContainer_sequence_of[1] = {
  { &hf_ivi_RoadConfigurationContainer_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_RccPart },
};

static int
dissect_ivi_RoadConfigurationContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_RoadConfigurationContainer, ivi_RoadConfigurationContainer_sequence_of,
                                                  1, 16, TRUE);

  return offset;
}


static const per_sequence_t ivi_T_TcPartDetectionZoneIds_sequence_of[1] = {
  { &hf_ivi_tpDetectionZoneIds_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_Zid },
};

static int
dissect_ivi_T_TcPartDetectionZoneIds(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_TcPartDetectionZoneIds, ivi_T_TcPartDetectionZoneIds_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const per_sequence_t ivi_T_TcPartRelevanceZoneIds_sequence_of[1] = {
  { &hf_ivi_tpRelevanceZoneIds_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_Zid },
};

static int
dissect_ivi_T_TcPartRelevanceZoneIds(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_TcPartRelevanceZoneIds, ivi_T_TcPartRelevanceZoneIds_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const per_sequence_t ivi_T_TcPartDriverAwarenessZoneIds_sequence_of[1] = {
  { &hf_ivi_tpDriverAwarenessZoneIds_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_Zid },
};

static int
dissect_ivi_T_TcPartDriverAwarenessZoneIds(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_TcPartDriverAwarenessZoneIds, ivi_T_TcPartDriverAwarenessZoneIds_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const per_sequence_t ivi_T_TcPartText_sequence_of[1] = {
  { &hf_ivi_text_item       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_Text },
};

static int
dissect_ivi_T_TcPartText(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_T_TcPartText, ivi_T_TcPartText_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}


static const per_sequence_t ivi_TcPart_sequence[] = {
  { &hf_ivi_tpDetectionZoneIds, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_T_TcPartDetectionZoneIds },
  { &hf_ivi_tpRelevanceZoneIds, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_T_TcPartRelevanceZoneIds },
  { &hf_ivi_direction       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_Direction },
  { &hf_ivi_tpDriverAwarenessZoneIds, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_T_TcPartDriverAwarenessZoneIds },
  { &hf_ivi_minimumAwarenessTime, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_INTEGER_0_255 },
  { &hf_ivi_applicableLanes , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_8__OF_LanePosition },
  { &hf_ivi_layoutId        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_4_ },
  { &hf_ivi_preStoredlayoutId, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_INTEGER_1_64_ },
  { &hf_ivi_text            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_T_TcPartText },
  { &hf_ivi_data            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_OCTET_STRING },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_TcPart(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_TcPart, ivi_TcPart_sequence);

  return offset;
}


static const per_sequence_t ivi_TextContainer_sequence_of[1] = {
  { &hf_ivi_TextContainer_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_TcPart },
};

static int
dissect_ivi_TextContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_TextContainer, ivi_TextContainer_sequence_of,
                                                  1, 16, TRUE);

  return offset;
}



static int
dissect_ivi_INTEGER_10_73(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            10U, 73U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_INTEGER_10_265(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            10U, 265U, NULL, FALSE);

  return offset;
}



static int
dissect_ivi_INTEGER_1_8_(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 8U, NULL, TRUE);

  return offset;
}


static const value_string ivi_T_textScripting_vals[] = {
  {   0, "horizontal" },
  {   1, "vertical" },
  { 0, NULL }
};


static int
dissect_ivi_T_textScripting(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 1U, NULL, FALSE);

  return offset;
}


static const per_sequence_t ivi_LayoutComponent_sequence[] = {
  { &hf_ivi_lcLayoutComponentId, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_1_8_ },
  { &hf_ivi_height          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_10_73 },
  { &hf_ivi_width           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_10_265 },
  { &hf_ivi_x               , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_10_265 },
  { &hf_ivi_y               , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_10_73 },
  { &hf_ivi_textScripting   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_T_textScripting },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_LayoutComponent(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_LayoutComponent, ivi_LayoutComponent_sequence);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_4__OF_LayoutComponent_sequence_of[1] = {
  { &hf_ivi_layoutComponents_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_LayoutComponent },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_4__OF_LayoutComponent(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_4__OF_LayoutComponent, ivi_SEQUENCE_SIZE_1_4__OF_LayoutComponent_sequence_of,
                                                  1, 4, TRUE);

  return offset;
}


static const per_sequence_t ivi_LayoutContainer_sequence[] = {
  { &hf_ivi_layoutId        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_INTEGER_1_4_ },
  { &hf_ivi_height          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_INTEGER_10_73 },
  { &hf_ivi_width           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_ivi_INTEGER_10_265 },
  { &hf_ivi_layoutComponents, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_ivi_SEQUENCE_SIZE_1_4__OF_LayoutComponent },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_LayoutContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_LayoutContainer, ivi_LayoutContainer_sequence);

  return offset;
}


static const value_string ivi_IviContainer_vals[] = {
  {   0, "glc" },
  {   1, "giv" },
  {   2, "rcc" },
  {   3, "tc" },
  {   4, "lac" },
  { 0, NULL }
};

static const per_choice_t ivi_IviContainer_choice[] = {
  {   0, &hf_ivi_glc             , ASN1_EXTENSION_ROOT    , dissect_ivi_GeographicLocationContainer },
  {   1, &hf_ivi_giv             , ASN1_EXTENSION_ROOT    , dissect_ivi_GeneralIviContainer },
  {   2, &hf_ivi_rcc             , ASN1_EXTENSION_ROOT    , dissect_ivi_RoadConfigurationContainer },
  {   3, &hf_ivi_tc              , ASN1_EXTENSION_ROOT    , dissect_ivi_TextContainer },
  {   4, &hf_ivi_lac             , ASN1_EXTENSION_ROOT    , dissect_ivi_LayoutContainer },
  { 0, NULL, 0, NULL }
};

static int
dissect_ivi_IviContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_ivi_IviContainer, ivi_IviContainer_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t ivi_SEQUENCE_SIZE_1_8__OF_IviContainer_sequence_of[1] = {
  { &hf_ivi_optional_item   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_IviContainer },
};

static int
dissect_ivi_SEQUENCE_SIZE_1_8__OF_IviContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_ivi_SEQUENCE_SIZE_1_8__OF_IviContainer, ivi_SEQUENCE_SIZE_1_8__OF_IviContainer_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const per_sequence_t ivi_IviStructure_sequence[] = {
  { &hf_ivi_mandatory       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_ivi_IVIManagementContainer },
  { &hf_ivi_optional        , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_ivi_SEQUENCE_SIZE_1_8__OF_IviContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_ivi_IviStructure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 302 "./asn1/its/its.cnf"
  actx->private_data = (void*)wmem_new0(wmem_packet_scope(), its_private_data_t);
  col_set_str(actx->pinfo->cinfo, COL_PROTOCOL, "IVIM");
  col_set_str(actx->pinfo->cinfo, COL_INFO, "IVIM");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_ivi_IviStructure, ivi_IviStructure_sequence);

  return offset;
}

/*--- PDUs ---*/

static int dissect_ivi_IviStructure_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_ivi_IviStructure(tvb, offset, &asn1_ctx, tree, hf_ivi_ivi_IviStructure_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/* --- Module DSRC --- --- ---                                                */


static const value_string dsrc_RegionId_vals[] = {
  { noRegion, "noRegion" },
  { addGrpA, "addGrpA" },
  { addGrpB, "addGrpB" },
  { addGrpC, "addGrpC" },
  { 0, NULL }
};


static int
dissect_dsrc_RegionId(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, &((its_private_data_t*)actx->private_data)->region_id, FALSE);

  return offset;
}



static int
dissect_dsrc_T_regExtValue(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_open_type_pdu_new(tvb, offset, actx, tree, hf_index, dissect_regextval_pdu);

  return offset;
}


static const per_sequence_t dsrc_RegionalExtension_sequence[] = {
  { &hf_dsrc_regionId       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionId },
  { &hf_dsrc_regExtValue    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_T_regExtValue },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_RegionalExtension(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_RegionalExtension, dsrc_RegionalExtension_sequence);

  return offset;
}



static int
dissect_dsrc_MinuteOfTheYear(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 527040U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_DescriptiveName(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          1, 63, FALSE);

  return offset;
}



static int
dissect_dsrc_RoadRegulatorID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_IntersectionID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_IntersectionReferenceID_sequence[] = {
  { &hf_dsrc_region         , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_RoadRegulatorID },
  { &hf_dsrc_irId           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionID },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_IntersectionReferenceID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_IntersectionReferenceID, dsrc_IntersectionReferenceID_sequence);

  return offset;
}



static int
dissect_dsrc_MsgCount(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 127U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_IntersectionStatusObject(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 837 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields0[] = { // CHECKED
      &hf_dsrc_IntersectionStatusObject_manualControlIsEnabled,
      &hf_dsrc_IntersectionStatusObject_stopTimeIsActivated,
      &hf_dsrc_IntersectionStatusObject_failureFlash,
      &hf_dsrc_IntersectionStatusObject_preemptIsActive,
      &hf_dsrc_IntersectionStatusObject_signalPriorityIsActive,
      &hf_dsrc_IntersectionStatusObject_fixedTimeOperation,
      &hf_dsrc_IntersectionStatusObject_trafficDependentOperation,
      &hf_dsrc_IntersectionStatusObject_standbyOperation,
      NULL
    };
    const gint *fields1[] = { // CHECKED
      &hf_dsrc_IntersectionStatusObject_failureMode,
      &hf_dsrc_IntersectionStatusObject_off,
      &hf_dsrc_IntersectionStatusObject_recentMAPmessageUpdate,
      &hf_dsrc_IntersectionStatusObject_recentChangeInMAPassignedLanesIDsUsed,
      &hf_dsrc_IntersectionStatusObject_noValidMAPisAvailableAtThisTime,
      &hf_dsrc_IntersectionStatusObject_noValidSPATisAvailableAtThisTime,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_IntersectionStatusObject);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields0, ENC_BIG_ENDIAN);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 1, 1, fields1, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_DSecond(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_LaneID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_EnabledLaneList_sequence_of[1] = {
  { &hf_dsrc_EnabledLaneList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneID },
};

static int
dissect_dsrc_EnabledLaneList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_EnabledLaneList, dsrc_EnabledLaneList_sequence_of,
                                                  1, 16, FALSE);

  return offset;
}



static int
dissect_dsrc_SignalGroupID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const value_string dsrc_MovementPhaseState_vals[] = {
  {   0, "unavailable" },
  {   1, "dark" },
  {   2, "stop-Then-Proceed" },
  {   3, "stop-And-Remain" },
  {   4, "pre-Movement" },
  {   5, "permissive-Movement-Allowed" },
  {   6, "protected-Movement-Allowed" },
  {   7, "permissive-clearance" },
  {   8, "protected-clearance" },
  {   9, "caution-Conflicting-Traffic" },
  { 0, NULL }
};


static int
dissect_dsrc_MovementPhaseState(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     10, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_dsrc_TimeMark(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 36001U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_TimeIntervalConfidence(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 15U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_TimeChangeDetails_sequence[] = {
  { &hf_dsrc_startTime      , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_TimeMark },
  { &hf_dsrc_minEndTime     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_TimeMark },
  { &hf_dsrc_maxEndTime     , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_TimeMark },
  { &hf_dsrc_likelyTime     , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_TimeMark },
  { &hf_dsrc_tcdConfidence  , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_TimeIntervalConfidence },
  { &hf_dsrc_nextTime       , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_TimeMark },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_TimeChangeDetails(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_TimeChangeDetails, dsrc_TimeChangeDetails_sequence);

  return offset;
}


static const value_string dsrc_AdvisorySpeedType_vals[] = {
  {   0, "none" },
  {   1, "greenwave" },
  {   2, "ecoDrive" },
  {   3, "transit" },
  { 0, NULL }
};


static int
dissect_dsrc_AdvisorySpeedType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_dsrc_SpeedAdvice(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 500U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_ZoneLength(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 10000U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_RestrictionClassID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_AdvisorySpeedRegional_sequence_of[1] = {
  { &hf_dsrc_asRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_AdvisorySpeedRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_AdvisorySpeedRegional, dsrc_T_AdvisorySpeedRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_AdvisorySpeed_sequence[] = {
  { &hf_dsrc_asType         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_AdvisorySpeedType },
  { &hf_dsrc_asSpeed        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_SpeedAdvice },
  { &hf_dsrc_asConfidence   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_SpeedConfidence },
  { &hf_dsrc_distance       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_ZoneLength },
  { &hf_dsrc_class          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_RestrictionClassID },
  { &hf_dsrc_asRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_AdvisorySpeedRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_AdvisorySpeed(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 381 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_AdvisorySpeed;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_AdvisorySpeed, dsrc_AdvisorySpeed_sequence);

#line 385 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_AdvisorySpeedList_sequence_of[1] = {
  { &hf_dsrc_AdvisorySpeedList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_AdvisorySpeed },
};

static int
dissect_dsrc_AdvisorySpeedList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_AdvisorySpeedList, dsrc_AdvisorySpeedList_sequence_of,
                                                  1, 16, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_MovementEventRegional_sequence_of[1] = {
  { &hf_dsrc_meRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_MovementEventRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_MovementEventRegional, dsrc_T_MovementEventRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_MovementEvent_sequence[] = {
  { &hf_dsrc_eventState     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_MovementPhaseState },
  { &hf_dsrc_timing         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_TimeChangeDetails },
  { &hf_dsrc_speeds         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_AdvisorySpeedList },
  { &hf_dsrc_meRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_MovementEventRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_MovementEvent(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 453 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_MovementEvent;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_MovementEvent, dsrc_MovementEvent_sequence);

#line 457 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_MovementEventList_sequence_of[1] = {
  { &hf_dsrc_MovementEventList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_MovementEvent },
};

static int
dissect_dsrc_MovementEventList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_MovementEventList, dsrc_MovementEventList_sequence_of,
                                                  1, 16, FALSE);

  return offset;
}



static int
dissect_dsrc_LaneConnectionID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_WaitOnStopline(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_boolean(tvb, offset, actx, tree, hf_index, NULL);

  return offset;
}



static int
dissect_dsrc_PedestrianBicycleDetect(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_boolean(tvb, offset, actx, tree, hf_index, NULL);

  return offset;
}


static const per_sequence_t dsrc_T_ConnectionManeuverAssistRegional_sequence_of[1] = {
  { &hf_dsrc_cmaRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_ConnectionManeuverAssistRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_ConnectionManeuverAssistRegional, dsrc_T_ConnectionManeuverAssistRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_ConnectionManeuverAssist_sequence[] = {
  { &hf_dsrc_connectionID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneConnectionID },
  { &hf_dsrc_queueLength    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_ZoneLength },
  { &hf_dsrc_availableStorageLength, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_ZoneLength },
  { &hf_dsrc_waitOnStop     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_WaitOnStopline },
  { &hf_dsrc_pedBicycleDetect, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_PedestrianBicycleDetect },
  { &hf_dsrc_cmaRegional    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_ConnectionManeuverAssistRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_ConnectionManeuverAssist(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 399 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_ConnectionManeuverAssist;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_ConnectionManeuverAssist, dsrc_ConnectionManeuverAssist_sequence);

#line 403 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_ManeuverAssistList_sequence_of[1] = {
  { &hf_dsrc_ManeuverAssistList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_ConnectionManeuverAssist },
};

static int
dissect_dsrc_ManeuverAssistList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_ManeuverAssistList, dsrc_ManeuverAssistList_sequence_of,
                                                  1, 16, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_MovementStateRegional_sequence_of[1] = {
  { &hf_dsrc_msRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_MovementStateRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_MovementStateRegional, dsrc_T_MovementStateRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_MovementState_sequence[] = {
  { &hf_dsrc_movementName   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DescriptiveName },
  { &hf_dsrc_signalGroup    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_SignalGroupID },
  { &hf_dsrc_state_time_speed, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_MovementEventList },
  { &hf_dsrc_maneuverAssistList, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_ManeuverAssistList },
  { &hf_dsrc_msRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_MovementStateRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_MovementState(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 462 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_MovementState;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_MovementState, dsrc_MovementState_sequence);

#line 466 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_MovementList_sequence_of[1] = {
  { &hf_dsrc_MovementList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_MovementState },
};

static int
dissect_dsrc_MovementList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_MovementList, dsrc_MovementList_sequence_of,
                                                  1, 255, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_IntersectionStateRegional_sequence_of[1] = {
  { &hf_dsrc_isRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_IntersectionStateRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_IntersectionStateRegional, dsrc_T_IntersectionStateRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_IntersectionState_sequence[] = {
  { &hf_dsrc_name           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DescriptiveName },
  { &hf_dsrc_isId           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionReferenceID },
  { &hf_dsrc_revision       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_MsgCount },
  { &hf_dsrc_isStatus       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionStatusObject },
  { &hf_dsrc_moy            , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_MinuteOfTheYear },
  { &hf_dsrc_isTimeStamp    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DSecond },
  { &hf_dsrc_enabledLanes   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_EnabledLaneList },
  { &hf_dsrc_states         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_MovementList },
  { &hf_dsrc_maneuverAssistList, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_ManeuverAssistList },
  { &hf_dsrc_isRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_IntersectionStateRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_IntersectionState(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 426 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_IntersectionState;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_IntersectionState, dsrc_IntersectionState_sequence);

#line 430 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_IntersectionStateList_sequence_of[1] = {
  { &hf_dsrc_IntersectionStateList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionState },
};

static int
dissect_dsrc_IntersectionStateList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_IntersectionStateList, dsrc_IntersectionStateList_sequence_of,
                                                  1, 32, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_SPATRegional_sequence_of[1] = {
  { &hf_dsrc_spatRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_SPATRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_SPATRegional, dsrc_T_SPATRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_SPAT_sequence[] = {
  { &hf_dsrc_spatTimeStamp  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_MinuteOfTheYear },
  { &hf_dsrc_name           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DescriptiveName },
  { &hf_dsrc_spatIntersections, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionStateList },
  { &hf_dsrc_spatRegional   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_SPATRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_SPAT(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 332 "./asn1/its/its.cnf"
  its_private_data_t *regext = wmem_new0(wmem_packet_scope(), its_private_data_t);
  actx->private_data = (void*)regext;
  col_set_str(actx->pinfo->cinfo, COL_PROTOCOL, "SPATEM");
  col_set_str(actx->pinfo->cinfo, COL_INFO, "SPATEM");
  regext->type = Reg_SPAT;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_SPAT, dsrc_SPAT_sequence);

  return offset;
}



static int
dissect_dsrc_RequestID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const value_string dsrc_PriorityRequestType_vals[] = {
  {   0, "priorityRequestTypeReserved" },
  {   1, "priorityRequest" },
  {   2, "priorityRequestUpdate" },
  {   3, "priorityCancellation" },
  { 0, NULL }
};


static int
dissect_dsrc_PriorityRequestType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     4, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_dsrc_ApproachID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 15U, NULL, FALSE);

  return offset;
}


static const value_string dsrc_IntersectionAccessPoint_vals[] = {
  {   0, "lane" },
  {   1, "approach" },
  {   2, "connection" },
  { 0, NULL }
};

static const per_choice_t dsrc_IntersectionAccessPoint_choice[] = {
  {   0, &hf_dsrc_lane           , ASN1_EXTENSION_ROOT    , dissect_dsrc_LaneID },
  {   1, &hf_dsrc_approach       , ASN1_EXTENSION_ROOT    , dissect_dsrc_ApproachID },
  {   2, &hf_dsrc_connection     , ASN1_EXTENSION_ROOT    , dissect_dsrc_LaneConnectionID },
  { 0, NULL, 0, NULL }
};

static int
dissect_dsrc_IntersectionAccessPoint(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_dsrc_IntersectionAccessPoint, dsrc_IntersectionAccessPoint_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t dsrc_T_SignalRequestRegional_sequence_of[1] = {
  { &hf_dsrc_srRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_SignalRequestRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_SignalRequestRegional, dsrc_T_SignalRequestRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_SignalRequest_sequence[] = {
  { &hf_dsrc_srId           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionReferenceID },
  { &hf_dsrc_requestID      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_RequestID },
  { &hf_dsrc_requestType    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_PriorityRequestType },
  { &hf_dsrc_inBoundLane    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionAccessPoint },
  { &hf_dsrc_outBoundLane   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_IntersectionAccessPoint },
  { &hf_dsrc_srRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_SignalRequestRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_SignalRequest(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 543 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_SignalRequest;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_SignalRequest, dsrc_SignalRequest_sequence);

#line 547 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_T_SignalRequestPackageRegional_sequence_of[1] = {
  { &hf_dsrc_srpRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_SignalRequestPackageRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_SignalRequestPackageRegional, dsrc_T_SignalRequestPackageRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_SignalRequestPackage_sequence[] = {
  { &hf_dsrc_srpRequest     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_SignalRequest },
  { &hf_dsrc_minute         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_MinuteOfTheYear },
  { &hf_dsrc_second         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DSecond },
  { &hf_dsrc_duration       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DSecond },
  { &hf_dsrc_srpRegional    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_SignalRequestPackageRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_SignalRequestPackage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 534 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_SignalRequestPackage;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_SignalRequestPackage, dsrc_SignalRequestPackage_sequence);

#line 538 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_SignalRequestList_sequence_of[1] = {
  { &hf_dsrc_SignalRequestList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_SignalRequestPackage },
};

static int
dissect_dsrc_SignalRequestList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_SignalRequestList, dsrc_SignalRequestList_sequence_of,
                                                  1, 32, FALSE);

  return offset;
}



static int
dissect_dsrc_TemporaryID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       4, 4, FALSE, NULL);

  return offset;
}


static const value_string dsrc_VehicleID_vals[] = {
  {   0, "entityID" },
  {   1, "stationID" },
  { 0, NULL }
};

static const per_choice_t dsrc_VehicleID_choice[] = {
  {   0, &hf_dsrc_entityID       , ASN1_NO_EXTENSIONS     , dissect_dsrc_TemporaryID },
  {   1, &hf_dsrc_stationID      , ASN1_NO_EXTENSIONS     , dissect_its_StationID },
  { 0, NULL, 0, NULL }
};

static int
dissect_dsrc_VehicleID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_dsrc_VehicleID, dsrc_VehicleID_choice,
                                 NULL);

  return offset;
}


static const value_string dsrc_BasicVehicleRole_vals[] = {
  {   0, "basicVehicle" },
  {   1, "publicTransport" },
  {   2, "specialTransport" },
  {   3, "dangerousGoods" },
  {   4, "roadWork" },
  {   5, "roadRescue" },
  {   6, "emergency" },
  {   7, "safetyCar" },
  {   8, "none-unknown" },
  {   9, "truck" },
  {  10, "motorcycle" },
  {  11, "roadSideSource" },
  {  12, "police" },
  {  13, "fire" },
  {  14, "ambulance" },
  {  15, "dot" },
  {  16, "transit" },
  {  17, "slowMoving" },
  {  18, "stopNgo" },
  {  19, "cyclist" },
  {  20, "pedestrian" },
  {  21, "nonMotorized" },
  {  22, "military" },
  { 0, NULL }
};


static int
dissect_dsrc_BasicVehicleRole(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     23, NULL, TRUE, 0, NULL);

  return offset;
}


static const value_string dsrc_RequestSubRole_vals[] = {
  {   0, "requestSubRoleUnKnown" },
  {   1, "requestSubRole1" },
  {   2, "requestSubRole2" },
  {   3, "requestSubRole3" },
  {   4, "requestSubRole4" },
  {   5, "requestSubRole5" },
  {   6, "requestSubRole6" },
  {   7, "requestSubRole7" },
  {   8, "requestSubRole8" },
  {   9, "requestSubRole9" },
  {  10, "requestSubRole10" },
  {  11, "requestSubRole11" },
  {  12, "requestSubRole12" },
  {  13, "requestSubRole13" },
  {  14, "requestSubRole14" },
  {  15, "requestSubRoleReserved" },
  { 0, NULL }
};


static int
dissect_dsrc_RequestSubRole(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     16, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string dsrc_RequestImportanceLevel_vals[] = {
  {   0, "requestImportanceLevelUnKnown" },
  {   1, "requestImportanceLevel1" },
  {   2, "requestImportanceLevel2" },
  {   3, "requestImportanceLevel3" },
  {   4, "requestImportanceLevel4" },
  {   5, "requestImportanceLevel5" },
  {   6, "requestImportanceLevel6" },
  {   7, "requestImportanceLevel7" },
  {   8, "requestImportanceLevel8" },
  {   9, "requestImportanceLevel9" },
  {  10, "requestImportanceLevel10" },
  {  11, "requestImportanceLevel11" },
  {  12, "requestImportanceLevel12" },
  {  13, "requestImportanceLevel13" },
  {  14, "requestImportanceLevel14" },
  {  15, "requestImportanceReserved" },
  { 0, NULL }
};


static int
dissect_dsrc_RequestImportanceLevel(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     16, NULL, FALSE, 0, NULL);

  return offset;
}


static const value_string dsrc_VehicleType_vals[] = {
  {   0, "none" },
  {   1, "unknown" },
  {   2, "special" },
  {   3, "moto" },
  {   4, "car" },
  {   5, "carOther" },
  {   6, "bus" },
  {   7, "axleCnt2" },
  {   8, "axleCnt3" },
  {   9, "axleCnt4" },
  {  10, "axleCnt4Trailer" },
  {  11, "axleCnt5Trailer" },
  {  12, "axleCnt6Trailer" },
  {  13, "axleCnt5MultiTrailer" },
  {  14, "axleCnt6MultiTrailer" },
  {  15, "axleCnt7MultiTrailer" },
  { 0, NULL }
};


static int
dissect_dsrc_VehicleType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     16, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t dsrc_RequestorType_sequence[] = {
  { &hf_dsrc_role           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_BasicVehicleRole },
  { &hf_dsrc_subrole        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_RequestSubRole },
  { &hf_dsrc_rtRequest      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_RequestImportanceLevel },
  { &hf_dsrc_iso3883        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_erivdm_Iso3833VehicleType },
  { &hf_dsrc_hpmsType       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_VehicleType },
  { &hf_dsrc_rtRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_RegionalExtension },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_RequestorType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 507 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_RequestorType;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_RequestorType, dsrc_RequestorType_sequence);

#line 511 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}



static int
dissect_dsrc_Elevation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -4096, 61439U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_Position3DRegional_sequence_of[1] = {
  { &hf_dsrc_p3dRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_Position3DRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_Position3DRegional, dsrc_T_Position3DRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_Position3D_sequence[] = {
  { &hf_dsrc_lat            , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Latitude },
  { &hf_dsrc_long           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Longitude },
  { &hf_dsrc_elevation      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_Elevation },
  { &hf_dsrc_p3dRegional    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_Position3DRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_Position3D(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 489 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_Position3D;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_Position3D, dsrc_Position3D_sequence);

#line 493 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}



static int
dissect_dsrc_Angle(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 28800U, NULL, FALSE);

  return offset;
}


static const value_string dsrc_TransmissionState_vals[] = {
  {   0, "neutral" },
  {   1, "park" },
  {   2, "forwardGears" },
  {   3, "reverseGears" },
  {   4, "reserved1" },
  {   5, "reserved2" },
  {   6, "reserved3" },
  {   7, "unavailable" },
  { 0, NULL }
};


static int
dissect_dsrc_TransmissionState(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     8, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_dsrc_Velocity(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 8191U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_TransmissionAndSpeed_sequence[] = {
  { &hf_dsrc_transmisson    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_TransmissionState },
  { &hf_dsrc_tasSpeed       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Velocity },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_TransmissionAndSpeed(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_TransmissionAndSpeed, dsrc_TransmissionAndSpeed_sequence);

  return offset;
}


static const per_sequence_t dsrc_RequestorPositionVector_sequence[] = {
  { &hf_dsrc_rpvPosition    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_Position3D },
  { &hf_dsrc_heading        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_Angle },
  { &hf_dsrc_rpvSpeed       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_TransmissionAndSpeed },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_RequestorPositionVector(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_RequestorPositionVector, dsrc_RequestorPositionVector_sequence);

  return offset;
}



static int
dissect_dsrc_TransitVehicleStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1027 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     8, 8, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_dsrc_TransitVehicleStatus_loading,
      &hf_dsrc_TransitVehicleStatus_anADAuse,
      &hf_dsrc_TransitVehicleStatus_aBikeLoad,
      &hf_dsrc_TransitVehicleStatus_doorOpen,
      &hf_dsrc_TransitVehicleStatus_charging,
      &hf_dsrc_TransitVehicleStatus_atStopLine,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_TransitVehicleStatus);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const value_string dsrc_TransitVehicleOccupancy_vals[] = {
  {   0, "occupancyUnknown" },
  {   1, "occupancyEmpty" },
  {   2, "occupancyVeryLow" },
  {   3, "occupancyLow" },
  {   4, "occupancyMed" },
  {   5, "occupancyHigh" },
  {   6, "occupancyNearlyFull" },
  {   7, "occupancyFull" },
  { 0, NULL }
};


static int
dissect_dsrc_TransitVehicleOccupancy(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     8, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_dsrc_DeltaTime(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -122, 121U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_RequestorDescriptionRegional_sequence_of[1] = {
  { &hf_dsrc_rdRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_RequestorDescriptionRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_RequestorDescriptionRegional, dsrc_T_RequestorDescriptionRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_RequestorDescription_sequence[] = {
  { &hf_dsrc_rdId           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_VehicleID },
  { &hf_dsrc_rdType         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_RequestorType },
  { &hf_dsrc_rdPosition     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_RequestorPositionVector },
  { &hf_dsrc_name           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DescriptiveName },
  { &hf_dsrc_routeName      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DescriptiveName },
  { &hf_dsrc_transitStatus  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_TransitVehicleStatus },
  { &hf_dsrc_transitOccupancy, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_TransitVehicleOccupancy },
  { &hf_dsrc_transitSchedule, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DeltaTime },
  { &hf_dsrc_rdRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_RequestorDescriptionRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_RequestorDescription(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 498 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_RequestorDescription;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_RequestorDescription, dsrc_RequestorDescription_sequence);

#line 502 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_T_SRMRegional_sequence_of[1] = {
  { &hf_dsrc_srmRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_SRMRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_SRMRegional, dsrc_T_SRMRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_SignalRequestMessage_sequence[] = {
  { &hf_dsrc_srmTimeStamp   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_MinuteOfTheYear },
  { &hf_dsrc_second         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_DSecond },
  { &hf_dsrc_sequenceNumber , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_MsgCount },
  { &hf_dsrc_requests       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_SignalRequestList },
  { &hf_dsrc_requestor      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_RequestorDescription },
  { &hf_dsrc_srmRegional    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_SRMRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_SignalRequestMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 316 "./asn1/its/its.cnf"
  its_private_data_t *regext = wmem_new0(wmem_packet_scope(), its_private_data_t);
  actx->private_data = (void*)regext;
  col_set_str(actx->pinfo->cinfo, COL_PROTOCOL, "SREM");
  col_set_str(actx->pinfo->cinfo, COL_INFO, "SREM");
  regext->type = Reg_SignalRequestMessage;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_SignalRequestMessage, dsrc_SignalRequestMessage_sequence);

  return offset;
}


static const per_sequence_t dsrc_SignalRequesterInfo_sequence[] = {
  { &hf_dsrc_sriId          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_VehicleID },
  { &hf_dsrc_sriRequest     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_RequestID },
  { &hf_dsrc_sequenceNumber , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_MsgCount },
  { &hf_dsrc_role           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_BasicVehicleRole },
  { &hf_dsrc_typeData       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_RequestorType },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_SignalRequesterInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_SignalRequesterInfo, dsrc_SignalRequesterInfo_sequence);

  return offset;
}


static const value_string dsrc_PrioritizationResponseStatus_vals[] = {
  {   0, "unknown" },
  {   1, "requested" },
  {   2, "processing" },
  {   3, "watchOtherTraffic" },
  {   4, "granted" },
  {   5, "rejected" },
  {   6, "maxPresence" },
  {   7, "reserviceLocked" },
  { 0, NULL }
};


static int
dissect_dsrc_PrioritizationResponseStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     8, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t dsrc_T_SignalStatusPackageRegional_sequence_of[1] = {
  { &hf_dsrc_sspRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_SignalStatusPackageRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_SignalStatusPackageRegional, dsrc_T_SignalStatusPackageRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_SignalStatusPackage_sequence[] = {
  { &hf_dsrc_requester      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_SignalRequesterInfo },
  { &hf_dsrc_inboundOn      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionAccessPoint },
  { &hf_dsrc_outboundOn     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_IntersectionAccessPoint },
  { &hf_dsrc_minute         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_MinuteOfTheYear },
  { &hf_dsrc_second         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DSecond },
  { &hf_dsrc_duration       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DSecond },
  { &hf_dsrc_sspStatus      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_PrioritizationResponseStatus },
  { &hf_dsrc_sspRegional    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_SignalStatusPackageRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_SignalStatusPackage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 552 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_SignalStatusPackage;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_SignalStatusPackage, dsrc_SignalStatusPackage_sequence);

#line 556 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_SignalStatusPackageList_sequence_of[1] = {
  { &hf_dsrc_SignalStatusPackageList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_SignalStatusPackage },
};

static int
dissect_dsrc_SignalStatusPackageList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_SignalStatusPackageList, dsrc_SignalStatusPackageList_sequence_of,
                                                  1, 32, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_SignalStatusRegional_sequence_of[1] = {
  { &hf_dsrc_ssRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_SignalStatusRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_SignalStatusRegional, dsrc_T_SignalStatusRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_SignalStatus_sequence[] = {
  { &hf_dsrc_sequenceNumber , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_MsgCount },
  { &hf_dsrc_ssId           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionReferenceID },
  { &hf_dsrc_sigStatus      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_SignalStatusPackageList },
  { &hf_dsrc_ssRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_SignalStatusRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_SignalStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 561 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_SignalStatus;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_SignalStatus, dsrc_SignalStatus_sequence);

#line 565 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_SignalStatusList_sequence_of[1] = {
  { &hf_dsrc_SignalStatusList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_SignalStatus },
};

static int
dissect_dsrc_SignalStatusList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_SignalStatusList, dsrc_SignalStatusList_sequence_of,
                                                  1, 32, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_SSMRegional_sequence_of[1] = {
  { &hf_dsrc_ssmRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_SSMRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_SSMRegional, dsrc_T_SSMRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_SignalStatusMessage_sequence[] = {
  { &hf_dsrc_ssmTimeStamp   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_MinuteOfTheYear },
  { &hf_dsrc_second         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_DSecond },
  { &hf_dsrc_sequenceNumber , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_MsgCount },
  { &hf_dsrc_ssmStatus      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_SignalStatusList },
  { &hf_dsrc_ssmRegional    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_SSMRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_SignalStatusMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 308 "./asn1/its/its.cnf"
  its_private_data_t *regext = wmem_new0(wmem_packet_scope(), its_private_data_t);
  actx->private_data = (void*)regext;
  col_set_str(actx->pinfo->cinfo, COL_PROTOCOL, "SSEM");
  col_set_str(actx->pinfo->cinfo, COL_INFO, "SSEM");
  regext->type = Reg_SignalStatusMessage;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_SignalStatusMessage, dsrc_SignalStatusMessage_sequence);

  return offset;
}


static const value_string dsrc_LayerType_vals[] = {
  {   0, "none" },
  {   1, "mixedContent" },
  {   2, "generalMapData" },
  {   3, "intersectionData" },
  {   4, "curveData" },
  {   5, "roadwaySectionData" },
  {   6, "parkingAreaData" },
  {   7, "sharedLaneData" },
  { 0, NULL }
};


static int
dissect_dsrc_LayerType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     8, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_dsrc_LayerID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 100U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_LaneWidth(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 32767U, NULL, FALSE);

  return offset;
}


static const value_string dsrc_SpeedLimitType_vals[] = {
  {   0, "unknown" },
  {   1, "maxSpeedInSchoolZone" },
  {   2, "maxSpeedInSchoolZoneWhenChildrenArePresent" },
  {   3, "maxSpeedInConstructionZone" },
  {   4, "vehicleMinSpeed" },
  {   5, "vehicleMaxSpeed" },
  {   6, "vehicleNightMaxSpeed" },
  {   7, "truckMinSpeed" },
  {   8, "truckMaxSpeed" },
  {   9, "truckNightMaxSpeed" },
  {  10, "vehiclesWithTrailersMinSpeed" },
  {  11, "vehiclesWithTrailersMaxSpeed" },
  {  12, "vehiclesWithTrailersNightMaxSpeed" },
  { 0, NULL }
};


static int
dissect_dsrc_SpeedLimitType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     13, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t dsrc_RegulatorySpeedLimit_sequence[] = {
  { &hf_dsrc_rslType        , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_SpeedLimitType },
  { &hf_dsrc_rslSpeed       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Velocity },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_RegulatorySpeedLimit(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_RegulatorySpeedLimit, dsrc_RegulatorySpeedLimit_sequence);

  return offset;
}


static const per_sequence_t dsrc_SpeedLimitList_sequence_of[1] = {
  { &hf_dsrc_SpeedLimitList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegulatorySpeedLimit },
};

static int
dissect_dsrc_SpeedLimitList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_SpeedLimitList, dsrc_SpeedLimitList_sequence_of,
                                                  1, 9, FALSE);

  return offset;
}



static int
dissect_dsrc_LaneDirection(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 917 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     2, 2, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_dsrc_LaneDirection_ingressPath,
      &hf_dsrc_LaneDirection_egressPath,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_LaneDirection);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_LaneSharing(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1002 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     10, 10, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields0[] = { // CHECKED
      &hf_dsrc_LaneSharing_overlappingLaneDescriptionProvided,
      &hf_dsrc_LaneSharing_multipleLanesTreatedAsOneLane,
      &hf_dsrc_LaneSharing_otherNonMotorizedTrafficTypes,
      &hf_dsrc_LaneSharing_individualMotorizedVehicleTraffic,
      &hf_dsrc_LaneSharing_busVehicleTraffic,
      &hf_dsrc_LaneSharing_taxiVehicleTraffic,
      &hf_dsrc_LaneSharing_pedestriansTraffic,
      &hf_dsrc_LaneSharing_cyclistVehicleTraffic,
      NULL
    };
    const gint *fields1[] = { // CHECKED
      &hf_dsrc_LaneSharing_trackedVehicleTraffic,
      &hf_dsrc_LaneSharing_pedestrianTraffic,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_LaneSharing);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields0, ENC_BIG_ENDIAN);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 1, 1, fields1, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_LaneAttributes_Vehicle(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 739 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     8, 8, TRUE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_dsrc_LaneAttributes_Vehicle_isVehicleRevocableLane,
      &hf_dsrc_LaneAttributes_Vehicle_isVehicleFlyOverLane,
      &hf_dsrc_LaneAttributes_Vehicle_hovLaneUseOnly,
      &hf_dsrc_LaneAttributes_Vehicle_restrictedToBusUse,
      &hf_dsrc_LaneAttributes_Vehicle_restrictedToTaxiUse,
      &hf_dsrc_LaneAttributes_Vehicle_restrictedFromPublicUse,
      &hf_dsrc_LaneAttributes_Vehicle_hasIRbeaconCoverage,
      &hf_dsrc_LaneAttributes_Vehicle_permissionOnRequest,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_LaneAttributes_Vehicle);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_LaneAttributes_Crosswalk(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 893 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields0[] = { // CHECKED
      &hf_dsrc_LaneAttributes_Crosswalk_crosswalkRevocableLane,
      &hf_dsrc_LaneAttributes_Crosswalk_bicyleUseAllowed,
      &hf_dsrc_LaneAttributes_Crosswalk_isXwalkFlyOverLane,
      &hf_dsrc_LaneAttributes_Crosswalk_fixedCycleTime,
      &hf_dsrc_LaneAttributes_Crosswalk_biDirectionalCycleTimes,
      &hf_dsrc_LaneAttributes_Crosswalk_hasPushToWalkButton,
      &hf_dsrc_LaneAttributes_Crosswalk_audioSupport,
      &hf_dsrc_LaneAttributes_Crosswalk_rfSignalRequestPresent,
      NULL
    };
    const gint *fields1[] = { // CHECKED
      &hf_dsrc_LaneAttributes_Crosswalk_unsignalizedSegmentsPresent,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_LaneAttributes_Crosswalk);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields0, ENC_BIG_ENDIAN);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 1, 1, fields1, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_LaneAttributes_Bike(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 930 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_dsrc_LaneAttributes_Bike_bikeRevocableLane,
      &hf_dsrc_LaneAttributes_Bike_pedestrianUseAllowed,
      &hf_dsrc_LaneAttributes_Bike_isBikeFlyOverLane,
      &hf_dsrc_LaneAttributes_Bike_fixedCycleTime,
      &hf_dsrc_LaneAttributes_Bike_biDirectionalCycleTimes,
      &hf_dsrc_LaneAttributes_Bike_isolatedByBarrier,
      &hf_dsrc_LaneAttributes_Bike_unsignalizedSegmentsPresent,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_LaneAttributes_Bike);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_LaneAttributes_Sidewalk(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1044 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_dsrc_LaneAttributes_Sidewalk_sidewalk_RevocableLane,
      &hf_dsrc_LaneAttributes_Sidewalk_bicyleUseAllowed,
      &hf_dsrc_LaneAttributes_Sidewalk_isSidewalkFlyOverLane,
      &hf_dsrc_LaneAttributes_Sidewalk_walkBikes,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_LaneAttributes_Sidewalk);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_LaneAttributes_Barrier(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 699 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields0[] = { // CHECKED
      &hf_dsrc_LaneAttributes_Barrier_median_RevocableLane,
      &hf_dsrc_LaneAttributes_Barrier_median,
      &hf_dsrc_LaneAttributes_Barrier_whiteLineHashing,
      &hf_dsrc_LaneAttributes_Barrier_stripedLines,
      &hf_dsrc_LaneAttributes_Barrier_doubleStripedLines,
      &hf_dsrc_LaneAttributes_Barrier_trafficCones,
      &hf_dsrc_LaneAttributes_Barrier_constructionBarrier,
      &hf_dsrc_LaneAttributes_Barrier_trafficChannels,
      NULL
    };
    const gint *fields1[] = { // CHECKED
      &hf_dsrc_LaneAttributes_Barrier_lowCurbs,
      &hf_dsrc_LaneAttributes_Barrier_highCurbs,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_LaneAttributes_Barrier);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields0, ENC_BIG_ENDIAN);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 1, 1, fields1, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_LaneAttributes_Striping(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 985 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesRevocableLane,
      &hf_dsrc_LaneAttributes_Striping_stripeDrawOnLeft,
      &hf_dsrc_LaneAttributes_Striping_stripeDrawOnRight,
      &hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesLeft,
      &hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesRight,
      &hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesAhead,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_LaneAttributes_Striping);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_LaneAttributes_TrackedVehicle(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 771 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_dsrc_LaneAttributes_TrackedVehicle_spec_RevocableLane,
      &hf_dsrc_LaneAttributes_TrackedVehicle_spec_commuterRailRoadTrack,
      &hf_dsrc_LaneAttributes_TrackedVehicle_spec_lightRailRoadTrack,
      &hf_dsrc_LaneAttributes_TrackedVehicle_spec_heavyRailRoadTrack,
      &hf_dsrc_LaneAttributes_TrackedVehicle_spec_otherRailType,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_LaneAttributes_TrackedVehicle);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_LaneAttributes_Parking(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 948 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     16, 16, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_dsrc_LaneAttributes_Parking_parkingRevocableLane,
      &hf_dsrc_LaneAttributes_Parking_parallelParkingInUse,
      &hf_dsrc_LaneAttributes_Parking_headInParkingInUse,
      &hf_dsrc_LaneAttributes_Parking_doNotParkZone,
      &hf_dsrc_LaneAttributes_Parking_parkingForBusUse,
      &hf_dsrc_LaneAttributes_Parking_parkingForTaxiUse,
      &hf_dsrc_LaneAttributes_Parking_noPublicParkingUse,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_LaneAttributes_Parking);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const value_string dsrc_LaneTypeAttributes_vals[] = {
  {   0, "vehicle" },
  {   1, "crosswalk" },
  {   2, "bikeLane" },
  {   3, "sidewalk" },
  {   4, "median" },
  {   5, "striping" },
  {   6, "trackedVehicle" },
  {   7, "parking" },
  { 0, NULL }
};

static const per_choice_t dsrc_LaneTypeAttributes_choice[] = {
  {   0, &hf_dsrc_vehicle        , ASN1_EXTENSION_ROOT    , dissect_dsrc_LaneAttributes_Vehicle },
  {   1, &hf_dsrc_crosswalk      , ASN1_EXTENSION_ROOT    , dissect_dsrc_LaneAttributes_Crosswalk },
  {   2, &hf_dsrc_bikeLane       , ASN1_EXTENSION_ROOT    , dissect_dsrc_LaneAttributes_Bike },
  {   3, &hf_dsrc_sidewalk       , ASN1_EXTENSION_ROOT    , dissect_dsrc_LaneAttributes_Sidewalk },
  {   4, &hf_dsrc_median         , ASN1_EXTENSION_ROOT    , dissect_dsrc_LaneAttributes_Barrier },
  {   5, &hf_dsrc_striping       , ASN1_EXTENSION_ROOT    , dissect_dsrc_LaneAttributes_Striping },
  {   6, &hf_dsrc_trackedVehicle , ASN1_EXTENSION_ROOT    , dissect_dsrc_LaneAttributes_TrackedVehicle },
  {   7, &hf_dsrc_parking        , ASN1_EXTENSION_ROOT    , dissect_dsrc_LaneAttributes_Parking },
  { 0, NULL, 0, NULL }
};

static int
dissect_dsrc_LaneTypeAttributes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_dsrc_LaneTypeAttributes, dsrc_LaneTypeAttributes_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t dsrc_LaneAttributes_sequence[] = {
  { &hf_dsrc_directionalUse , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneDirection },
  { &hf_dsrc_sharedWith     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneSharing },
  { &hf_dsrc_laneType       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneTypeAttributes },
  { &hf_dsrc_laRegional     , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_RegionalExtension },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_LaneAttributes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 435 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_LaneAttributes;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_LaneAttributes, dsrc_LaneAttributes_sequence);

#line 439 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}



static int
dissect_dsrc_AllowedManeuvers(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 866 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     12, 12, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields0[] = { // CHECKED
      &hf_dsrc_AllowedManeuvers_maneuverStraightAllowed,
      &hf_dsrc_AllowedManeuvers_maneuverLeftAllowed,
      &hf_dsrc_AllowedManeuvers_maneuverRightAllowed,
      &hf_dsrc_AllowedManeuvers_maneuverUTurnAllowed,
      &hf_dsrc_AllowedManeuvers_maneuverLeftTurnOnRedAllowed,
      &hf_dsrc_AllowedManeuvers_maneuverRightTurnOnRedAllowed,
      &hf_dsrc_AllowedManeuvers_maneuverLaneChangeAllowed,
      &hf_dsrc_AllowedManeuvers_maneuverNoStoppingAllowed,
      NULL
    };
    const gint *fields1[] = { // CHECKED
      &hf_dsrc_AllowedManeuvers_yieldAllwaysRequired,
      &hf_dsrc_AllowedManeuvers_goWithHalt,
      &hf_dsrc_AllowedManeuvers_caution,
      &hf_dsrc_AllowedManeuvers_reserved1,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_dsrc_AllowedManeuvers);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields0, ENC_BIG_ENDIAN);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 1, 1, fields1, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_dsrc_Offset_B10(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -512, 511U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_Node_XY_20b_sequence[] = {
  { &hf_dsrc_n20bX          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B10 },
  { &hf_dsrc_n20bY          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B10 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_Node_XY_20b(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_Node_XY_20b, dsrc_Node_XY_20b_sequence);

  return offset;
}



static int
dissect_dsrc_Offset_B11(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -1024, 1023U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_Node_XY_22b_sequence[] = {
  { &hf_dsrc_n22bX          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B11 },
  { &hf_dsrc_n22bY          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B11 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_Node_XY_22b(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_Node_XY_22b, dsrc_Node_XY_22b_sequence);

  return offset;
}



static int
dissect_dsrc_Offset_B12(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -2048, 2047U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_Node_XY_24b_sequence[] = {
  { &hf_dsrc_n24bX          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B12 },
  { &hf_dsrc_n24bY          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B12 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_Node_XY_24b(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_Node_XY_24b, dsrc_Node_XY_24b_sequence);

  return offset;
}



static int
dissect_dsrc_Offset_B13(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -4096, 4095U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_Node_XY_26b_sequence[] = {
  { &hf_dsrc_n26bX          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B13 },
  { &hf_dsrc_n26bY          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B13 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_Node_XY_26b(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_Node_XY_26b, dsrc_Node_XY_26b_sequence);

  return offset;
}



static int
dissect_dsrc_Offset_B14(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -8192, 8191U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_Node_XY_28b_sequence[] = {
  { &hf_dsrc_n28bX          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B14 },
  { &hf_dsrc_n28bY          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B14 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_Node_XY_28b(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_Node_XY_28b, dsrc_Node_XY_28b_sequence);

  return offset;
}



static int
dissect_dsrc_Offset_B16(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -32768, 32767U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_Node_XY_32b_sequence[] = {
  { &hf_dsrc_n32bX          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B16 },
  { &hf_dsrc_n32bY          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Offset_B16 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_Node_XY_32b(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_Node_XY_32b, dsrc_Node_XY_32b_sequence);

  return offset;
}


static const per_sequence_t dsrc_Node_LLmD_64b_sequence[] = {
  { &hf_dsrc_lon            , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Longitude },
  { &hf_dsrc_lat            , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Latitude },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_Node_LLmD_64b(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_Node_LLmD_64b, dsrc_Node_LLmD_64b_sequence);

  return offset;
}


static const value_string dsrc_NodeOffsetPointXY_vals[] = {
  {   0, "node-XY1" },
  {   1, "node-XY2" },
  {   2, "node-XY3" },
  {   3, "node-XY4" },
  {   4, "node-XY5" },
  {   5, "node-XY6" },
  {   6, "node-LatLon" },
  {   7, "regional" },
  { 0, NULL }
};

static const per_choice_t dsrc_NodeOffsetPointXY_choice[] = {
  {   0, &hf_dsrc_node_XY1       , ASN1_NO_EXTENSIONS     , dissect_dsrc_Node_XY_20b },
  {   1, &hf_dsrc_node_XY2       , ASN1_NO_EXTENSIONS     , dissect_dsrc_Node_XY_22b },
  {   2, &hf_dsrc_node_XY3       , ASN1_NO_EXTENSIONS     , dissect_dsrc_Node_XY_24b },
  {   3, &hf_dsrc_node_XY4       , ASN1_NO_EXTENSIONS     , dissect_dsrc_Node_XY_26b },
  {   4, &hf_dsrc_node_XY5       , ASN1_NO_EXTENSIONS     , dissect_dsrc_Node_XY_28b },
  {   5, &hf_dsrc_node_XY6       , ASN1_NO_EXTENSIONS     , dissect_dsrc_Node_XY_32b },
  {   6, &hf_dsrc_node_LatLon    , ASN1_NO_EXTENSIONS     , dissect_dsrc_Node_LLmD_64b },
  {   7, &hf_dsrc_nopxyRegional  , ASN1_NO_EXTENSIONS     , dissect_dsrc_RegionalExtension },
  { 0, NULL, 0, NULL }
};

static int
dissect_dsrc_NodeOffsetPointXY(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 480 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_NodeOffsetPointXY;

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_dsrc_NodeOffsetPointXY, dsrc_NodeOffsetPointXY_choice,
                                 NULL);

#line 484 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const value_string dsrc_NodeAttributeXY_vals[] = {
  {   0, "reserved" },
  {   1, "stopLine" },
  {   2, "roundedCapStyleA" },
  {   3, "roundedCapStyleB" },
  {   4, "mergePoint" },
  {   5, "divergePoint" },
  {   6, "downstreamStopLine" },
  {   7, "downstreamStartNode" },
  {   8, "closedToTraffic" },
  {   9, "safeIsland" },
  {  10, "curbPresentAtStepOff" },
  {  11, "hydrantPresent" },
  { 0, NULL }
};


static int
dissect_dsrc_NodeAttributeXY(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     12, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t dsrc_NodeAttributeXYList_sequence_of[1] = {
  { &hf_dsrc_NodeAttributeXYList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_NodeAttributeXY },
};

static int
dissect_dsrc_NodeAttributeXYList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_NodeAttributeXYList, dsrc_NodeAttributeXYList_sequence_of,
                                                  1, 8, FALSE);

  return offset;
}


static const value_string dsrc_SegmentAttributeXY_vals[] = {
  {   0, "reserved" },
  {   1, "doNotBlock" },
  {   2, "whiteLine" },
  {   3, "mergingLaneLeft" },
  {   4, "mergingLaneRight" },
  {   5, "curbOnLeft" },
  {   6, "curbOnRight" },
  {   7, "loadingzoneOnLeft" },
  {   8, "loadingzoneOnRight" },
  {   9, "turnOutPointOnLeft" },
  {  10, "turnOutPointOnRight" },
  {  11, "adjacentParkingOnLeft" },
  {  12, "adjacentParkingOnRight" },
  {  13, "adjacentBikeLaneOnLeft" },
  {  14, "adjacentBikeLaneOnRight" },
  {  15, "sharedBikeLane" },
  {  16, "bikeBoxInFront" },
  {  17, "transitStopOnLeft" },
  {  18, "transitStopOnRight" },
  {  19, "transitStopInLane" },
  {  20, "sharedWithTrackedVehicle" },
  {  21, "safeIsland" },
  {  22, "lowCurbsPresent" },
  {  23, "rumbleStripPresent" },
  {  24, "audibleSignalingPresent" },
  {  25, "adaptiveTimingPresent" },
  {  26, "rfSignalRequestPresent" },
  {  27, "partialCurbIntrusion" },
  {  28, "taperToLeft" },
  {  29, "taperToRight" },
  {  30, "taperToCenterLine" },
  {  31, "parallelParking" },
  {  32, "headInParking" },
  {  33, "freeParking" },
  {  34, "timeRestrictionsOnParking" },
  {  35, "costToPark" },
  {  36, "midBlockCurbPresent" },
  {  37, "unEvenPavementPresent" },
  { 0, NULL }
};


static int
dissect_dsrc_SegmentAttributeXY(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     38, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t dsrc_SegmentAttributeXYList_sequence_of[1] = {
  { &hf_dsrc_SegmentAttributeXYList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_SegmentAttributeXY },
};

static int
dissect_dsrc_SegmentAttributeXYList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_SegmentAttributeXYList, dsrc_SegmentAttributeXYList_sequence_of,
                                                  1, 8, FALSE);

  return offset;
}



static int
dissect_dsrc_DeltaAngle(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -150, 150U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_RoadwayCrownAngle(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -128, 127U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_MergeDivergeNodeAngle(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -180, 180U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_LaneDataAttributeRegional_sequence_of[1] = {
  { &hf_dsrc_ldaRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_LaneDataAttributeRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_LaneDataAttributeRegional, dsrc_T_LaneDataAttributeRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const value_string dsrc_LaneDataAttribute_vals[] = {
  {   0, "pathEndPointAngle" },
  {   1, "laneCrownPointCenter" },
  {   2, "laneCrownPointLeft" },
  {   3, "laneCrownPointRight" },
  {   4, "laneAngle" },
  {   5, "speedLimits" },
  {   6, "regional" },
  { 0, NULL }
};

static const per_choice_t dsrc_LaneDataAttribute_choice[] = {
  {   0, &hf_dsrc_pathEndPointAngle, ASN1_EXTENSION_ROOT    , dissect_dsrc_DeltaAngle },
  {   1, &hf_dsrc_laneCrownPointCenter, ASN1_EXTENSION_ROOT    , dissect_dsrc_RoadwayCrownAngle },
  {   2, &hf_dsrc_laneCrownPointLeft, ASN1_EXTENSION_ROOT    , dissect_dsrc_RoadwayCrownAngle },
  {   3, &hf_dsrc_laneCrownPointRight, ASN1_EXTENSION_ROOT    , dissect_dsrc_RoadwayCrownAngle },
  {   4, &hf_dsrc_laneAngle      , ASN1_EXTENSION_ROOT    , dissect_dsrc_MergeDivergeNodeAngle },
  {   5, &hf_dsrc_speedLimits    , ASN1_EXTENSION_ROOT    , dissect_dsrc_SpeedLimitList },
  {   6, &hf_dsrc_ldaRegional    , ASN1_EXTENSION_ROOT    , dissect_dsrc_T_LaneDataAttributeRegional },
  { 0, NULL, 0, NULL }
};

static int
dissect_dsrc_LaneDataAttribute(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 444 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_LaneDataAttribute;

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_dsrc_LaneDataAttribute, dsrc_LaneDataAttribute_choice,
                                 NULL);

#line 448 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_LaneDataAttributeList_sequence_of[1] = {
  { &hf_dsrc_LaneDataAttributeList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneDataAttribute },
};

static int
dissect_dsrc_LaneDataAttributeList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_LaneDataAttributeList, dsrc_LaneDataAttributeList_sequence_of,
                                                  1, 8, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_NodeAttributeSetXYRegional_sequence_of[1] = {
  { &hf_dsrc_nasxyRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_NodeAttributeSetXYRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_NodeAttributeSetXYRegional, dsrc_T_NodeAttributeSetXYRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_NodeAttributeSetXY_sequence[] = {
  { &hf_dsrc_localNode      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_NodeAttributeXYList },
  { &hf_dsrc_disabled       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_SegmentAttributeXYList },
  { &hf_dsrc_enabled        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_SegmentAttributeXYList },
  { &hf_dsrc_data           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_LaneDataAttributeList },
  { &hf_dsrc_dWidth         , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_Offset_B10 },
  { &hf_dsrc_dElevation     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_Offset_B10 },
  { &hf_dsrc_nasxyRegional  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_NodeAttributeSetXYRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_NodeAttributeSetXY(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 471 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_NodeAttributeSetXY;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_NodeAttributeSetXY, dsrc_NodeAttributeSetXY_sequence);

#line 475 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_NodeXY_sequence[] = {
  { &hf_dsrc_delta          , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_NodeOffsetPointXY },
  { &hf_dsrc_attributes     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_NodeAttributeSetXY },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_NodeXY(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_NodeXY, dsrc_NodeXY_sequence);

  return offset;
}


static const per_sequence_t dsrc_NodeSetXY_sequence_of[1] = {
  { &hf_dsrc_NodeSetXY_item , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_NodeXY },
};

static int
dissect_dsrc_NodeSetXY(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_NodeSetXY, dsrc_NodeSetXY_sequence_of,
                                                  2, 63, FALSE);

  return offset;
}



static int
dissect_dsrc_DrivenLineOffsetSm(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -2047, 2047U, NULL, FALSE);

  return offset;
}



static int
dissect_dsrc_DrivenLineOffsetLg(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -32767, 32767U, NULL, FALSE);

  return offset;
}


static const value_string dsrc_T_offsetXaxis_vals[] = {
  {   0, "small" },
  {   1, "large" },
  { 0, NULL }
};

static const per_choice_t dsrc_T_offsetXaxis_choice[] = {
  {   0, &hf_dsrc_small          , ASN1_NO_EXTENSIONS     , dissect_dsrc_DrivenLineOffsetSm },
  {   1, &hf_dsrc_large          , ASN1_NO_EXTENSIONS     , dissect_dsrc_DrivenLineOffsetLg },
  { 0, NULL, 0, NULL }
};

static int
dissect_dsrc_T_offsetXaxis(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_dsrc_T_offsetXaxis, dsrc_T_offsetXaxis_choice,
                                 NULL);

  return offset;
}


static const value_string dsrc_T_offsetYaxis_vals[] = {
  {   0, "small" },
  {   1, "large" },
  { 0, NULL }
};

static const per_choice_t dsrc_T_offsetYaxis_choice[] = {
  {   0, &hf_dsrc_small          , ASN1_NO_EXTENSIONS     , dissect_dsrc_DrivenLineOffsetSm },
  {   1, &hf_dsrc_large          , ASN1_NO_EXTENSIONS     , dissect_dsrc_DrivenLineOffsetLg },
  { 0, NULL, 0, NULL }
};

static int
dissect_dsrc_T_offsetYaxis(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_dsrc_T_offsetYaxis, dsrc_T_offsetYaxis_choice,
                                 NULL);

  return offset;
}



static int
dissect_dsrc_Scale_B12(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            -2048, 2047U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_ComputedLaneRegional_sequence_of[1] = {
  { &hf_dsrc_clRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_ComputedLaneRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_ComputedLaneRegional, dsrc_T_ComputedLaneRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_ComputedLane_sequence[] = {
  { &hf_dsrc_referenceLaneId, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneID },
  { &hf_dsrc_offsetXaxis    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_T_offsetXaxis },
  { &hf_dsrc_offsetYaxis    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_T_offsetYaxis },
  { &hf_dsrc_rotateXY       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_Angle },
  { &hf_dsrc_scaleXaxis     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_Scale_B12 },
  { &hf_dsrc_scaleYaxis     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_Scale_B12 },
  { &hf_dsrc_clRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_ComputedLaneRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_ComputedLane(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 390 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_ComputedLane;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_ComputedLane, dsrc_ComputedLane_sequence);

#line 394 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const value_string dsrc_NodeListXY_vals[] = {
  {   0, "nodes" },
  {   1, "computed" },
  { 0, NULL }
};

static const per_choice_t dsrc_NodeListXY_choice[] = {
  {   0, &hf_dsrc_nodes          , ASN1_EXTENSION_ROOT    , dissect_dsrc_NodeSetXY },
  {   1, &hf_dsrc_computed       , ASN1_EXTENSION_ROOT    , dissect_dsrc_ComputedLane },
  { 0, NULL, 0, NULL }
};

static int
dissect_dsrc_NodeListXY(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_dsrc_NodeListXY, dsrc_NodeListXY_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t dsrc_ConnectingLane_sequence[] = {
  { &hf_dsrc_lane           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneID },
  { &hf_dsrc_maneuver       , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_AllowedManeuvers },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_ConnectingLane(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_ConnectingLane, dsrc_ConnectingLane_sequence);

  return offset;
}


static const per_sequence_t dsrc_Connection_sequence[] = {
  { &hf_dsrc_connectingLane , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_ConnectingLane },
  { &hf_dsrc_remoteIntersection, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_IntersectionReferenceID },
  { &hf_dsrc_signalGroup    , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_SignalGroupID },
  { &hf_dsrc_userClass      , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_RestrictionClassID },
  { &hf_dsrc_connectionID   , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_LaneConnectionID },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_Connection(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_Connection, dsrc_Connection_sequence);

  return offset;
}


static const per_sequence_t dsrc_ConnectsToList_sequence_of[1] = {
  { &hf_dsrc_ConnectsToList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_Connection },
};

static int
dissect_dsrc_ConnectsToList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_ConnectsToList, dsrc_ConnectsToList_sequence_of,
                                                  1, 16, FALSE);

  return offset;
}


static const per_sequence_t dsrc_OverlayLaneList_sequence_of[1] = {
  { &hf_dsrc_OverlayLaneList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneID },
};

static int
dissect_dsrc_OverlayLaneList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_OverlayLaneList, dsrc_OverlayLaneList_sequence_of,
                                                  1, 5, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_GenericLaneRegional_sequence_of[1] = {
  { &hf_dsrc_glRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_GenericLaneRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_GenericLaneRegional, dsrc_T_GenericLaneRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_GenericLane_sequence[] = {
  { &hf_dsrc_laneID         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneID },
  { &hf_dsrc_name           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DescriptiveName },
  { &hf_dsrc_ingressApproach, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_ApproachID },
  { &hf_dsrc_egressApproach , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_ApproachID },
  { &hf_dsrc_laneAttributes , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneAttributes },
  { &hf_dsrc_maneuvers      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_AllowedManeuvers },
  { &hf_dsrc_nodeList       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_NodeListXY },
  { &hf_dsrc_connectsTo     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_ConnectsToList },
  { &hf_dsrc_overlays       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_OverlayLaneList },
  { &hf_dsrc_glRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_GenericLaneRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_GenericLane(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 408 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_GenericLane;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_GenericLane, dsrc_GenericLane_sequence);

#line 412 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_LaneList_sequence_of[1] = {
  { &hf_dsrc_LaneList_item  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_GenericLane },
};

static int
dissect_dsrc_LaneList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_LaneList, dsrc_LaneList_sequence_of,
                                                  1, 255, FALSE);

  return offset;
}


static const per_sequence_t dsrc_SignalControlZone_sequence[] = {
  { &hf_dsrc_zone           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_SignalControlZone(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 570 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_SignalControlZone;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_SignalControlZone, dsrc_SignalControlZone_sequence);

#line 574 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_PreemptPriorityList_sequence_of[1] = {
  { &hf_dsrc_PreemptPriorityList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_SignalControlZone },
};

static int
dissect_dsrc_PreemptPriorityList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_PreemptPriorityList, dsrc_PreemptPriorityList_sequence_of,
                                                  1, 32, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_IntersectionGeometryRegional_sequence_of[1] = {
  { &hf_dsrc_igRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_IntersectionGeometryRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_IntersectionGeometryRegional, dsrc_T_IntersectionGeometryRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_IntersectionGeometry_sequence[] = {
  { &hf_dsrc_name           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DescriptiveName },
  { &hf_dsrc_igId           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionReferenceID },
  { &hf_dsrc_revision       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_MsgCount },
  { &hf_dsrc_refPoint       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_Position3D },
  { &hf_dsrc_laneWidth      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_LaneWidth },
  { &hf_dsrc_speedLimits    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_SpeedLimitList },
  { &hf_dsrc_laneSet        , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_LaneList },
  { &hf_dsrc_preemptPriorityData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_PreemptPriorityList },
  { &hf_dsrc_igRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_IntersectionGeometryRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_IntersectionGeometry(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 417 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_IntersectionGeometry;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_IntersectionGeometry, dsrc_IntersectionGeometry_sequence);

#line 421 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_IntersectionGeometryList_sequence_of[1] = {
  { &hf_dsrc_IntersectionGeometryList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_IntersectionGeometry },
};

static int
dissect_dsrc_IntersectionGeometryList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_IntersectionGeometryList, dsrc_IntersectionGeometryList_sequence_of,
                                                  1, 32, FALSE);

  return offset;
}



static int
dissect_dsrc_RoadSegmentID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t dsrc_RoadSegmentReferenceID_sequence[] = {
  { &hf_dsrc_region         , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_dsrc_RoadRegulatorID },
  { &hf_dsrc_rsrId          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RoadSegmentID },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_RoadSegmentReferenceID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_RoadSegmentReferenceID, dsrc_RoadSegmentReferenceID_sequence);

  return offset;
}


static const per_sequence_t dsrc_RoadLaneSetList_sequence_of[1] = {
  { &hf_dsrc_RoadLaneSetList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_GenericLane },
};

static int
dissect_dsrc_RoadLaneSetList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_RoadLaneSetList, dsrc_RoadLaneSetList_sequence_of,
                                                  1, 255, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_RoadSegmentRegional_sequence_of[1] = {
  { &hf_dsrc_rsRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_RoadSegmentRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_RoadSegmentRegional, dsrc_T_RoadSegmentRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_RoadSegment_sequence[] = {
  { &hf_dsrc_name           , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DescriptiveName },
  { &hf_dsrc_rsId           , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_RoadSegmentReferenceID },
  { &hf_dsrc_revision       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_MsgCount },
  { &hf_dsrc_refPoint       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_Position3D },
  { &hf_dsrc_laneWidth      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_LaneWidth },
  { &hf_dsrc_speedLimits    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_SpeedLimitList },
  { &hf_dsrc_roadLaneSet    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_RoadLaneSetList },
  { &hf_dsrc_rsRegional     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_RoadSegmentRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_RoadSegment(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 525 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_RoadSegment;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_RoadSegment, dsrc_RoadSegment_sequence);

#line 529 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_RoadSegmentList_sequence_of[1] = {
  { &hf_dsrc_RoadSegmentList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RoadSegment },
};

static int
dissect_dsrc_RoadSegmentList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_RoadSegmentList, dsrc_RoadSegmentList_sequence_of,
                                                  1, 32, FALSE);

  return offset;
}



static int
dissect_dsrc_IA5String_SIZE_1_255(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          1, 255, FALSE);

  return offset;
}


static const per_sequence_t dsrc_DataParameters_sequence[] = {
  { &hf_dsrc_processMethod  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_IA5String_SIZE_1_255 },
  { &hf_dsrc_processAgency  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_IA5String_SIZE_1_255 },
  { &hf_dsrc_lastCheckedDate, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_IA5String_SIZE_1_255 },
  { &hf_dsrc_geoidUsed      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_IA5String_SIZE_1_255 },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_DataParameters(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_DataParameters, dsrc_DataParameters_sequence);

  return offset;
}


static const value_string dsrc_RestrictionAppliesTo_vals[] = {
  {   0, "none" },
  {   1, "equippedTransit" },
  {   2, "equippedTaxis" },
  {   3, "equippedOther" },
  {   4, "emissionCompliant" },
  {   5, "equippedBicycle" },
  {   6, "weightCompliant" },
  {   7, "heightCompliant" },
  {   8, "pedestrians" },
  {   9, "slowMovingPersons" },
  {  10, "wheelchairUsers" },
  {  11, "visualDisabilities" },
  {  12, "audioDisabilities" },
  {  13, "otherUnknownDisabilities" },
  { 0, NULL }
};


static int
dissect_dsrc_RestrictionAppliesTo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     14, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t dsrc_T_RestrictionUserTypeRegional_sequence_of[1] = {
  { &hf_dsrc_rutRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_RestrictionUserTypeRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_RestrictionUserTypeRegional, dsrc_T_RestrictionUserTypeRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const value_string dsrc_RestrictionUserType_vals[] = {
  {   0, "basicType" },
  {   1, "regional" },
  { 0, NULL }
};

static const per_choice_t dsrc_RestrictionUserType_choice[] = {
  {   0, &hf_dsrc_basicType      , ASN1_EXTENSION_ROOT    , dissect_dsrc_RestrictionAppliesTo },
  {   1, &hf_dsrc_rutRegional    , ASN1_EXTENSION_ROOT    , dissect_dsrc_T_RestrictionUserTypeRegional },
  { 0, NULL, 0, NULL }
};

static int
dissect_dsrc_RestrictionUserType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 516 "./asn1/its/its.cnf"
  enum regext_type_enum save = ((its_private_data_t*)actx->private_data)->type;
  ((its_private_data_t*)actx->private_data)->type = Reg_RestrictionUserType;

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_dsrc_RestrictionUserType, dsrc_RestrictionUserType_choice,
                                 NULL);

#line 520 "./asn1/its/its.cnf"
  ((its_private_data_t*)actx->private_data)->type = save;

  return offset;
}


static const per_sequence_t dsrc_RestrictionUserTypeList_sequence_of[1] = {
  { &hf_dsrc_RestrictionUserTypeList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RestrictionUserType },
};

static int
dissect_dsrc_RestrictionUserTypeList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_RestrictionUserTypeList, dsrc_RestrictionUserTypeList_sequence_of,
                                                  1, 16, FALSE);

  return offset;
}


static const per_sequence_t dsrc_RestrictionClassAssignment_sequence[] = {
  { &hf_dsrc_scaId          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RestrictionClassID },
  { &hf_dsrc_users          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RestrictionUserTypeList },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_RestrictionClassAssignment(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_RestrictionClassAssignment, dsrc_RestrictionClassAssignment_sequence);

  return offset;
}


static const per_sequence_t dsrc_RestrictionClassList_sequence_of[1] = {
  { &hf_dsrc_RestrictionClassList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RestrictionClassAssignment },
};

static int
dissect_dsrc_RestrictionClassList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_RestrictionClassList, dsrc_RestrictionClassList_sequence_of,
                                                  1, 254, FALSE);

  return offset;
}


static const per_sequence_t dsrc_T_MAPRegional_sequence_of[1] = {
  { &hf_dsrc_mapRegional_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_dsrc_RegionalExtension },
};

static int
dissect_dsrc_T_MAPRegional(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_dsrc_T_MAPRegional, dsrc_T_MAPRegional_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t dsrc_MapData_sequence[] = {
  { &hf_dsrc_mdTimeStamp    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_MinuteOfTheYear },
  { &hf_dsrc_msgIssueRevision, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_MsgCount },
  { &hf_dsrc_layerType      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_LayerType },
  { &hf_dsrc_layerID        , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_LayerID },
  { &hf_dsrc_mdIntersections, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_IntersectionGeometryList },
  { &hf_dsrc_roadSegments   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_RoadSegmentList },
  { &hf_dsrc_dataParameters , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DataParameters },
  { &hf_dsrc_restrictionList, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_RestrictionClassList },
  { &hf_dsrc_mapRegional    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_T_MAPRegional },
  { NULL, 0, 0, NULL }
};

static int
dissect_dsrc_MapData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 324 "./asn1/its/its.cnf"
  its_private_data_t *regext = wmem_new0(wmem_packet_scope(), its_private_data_t);
  actx->private_data = (void*)regext;
  col_set_str(actx->pinfo->cinfo, COL_PROTOCOL, "MAPEM");
  col_set_str(actx->pinfo->cinfo, COL_INFO, "MAPEM");
  regext->type = Reg_MapData;

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_dsrc_MapData, dsrc_MapData_sequence);

  return offset;
}

/*--- PDUs ---*/

static int dissect_dsrc_SPAT_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_dsrc_SPAT(tvb, offset, &asn1_ctx, tree, hf_dsrc_dsrc_SPAT_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_dsrc_SignalRequestMessage_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_dsrc_SignalRequestMessage(tvb, offset, &asn1_ctx, tree, hf_dsrc_dsrc_SignalRequestMessage_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_dsrc_SignalStatusMessage_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_dsrc_SignalStatusMessage(tvb, offset, &asn1_ctx, tree, hf_dsrc_dsrc_SignalStatusMessage_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_dsrc_MapData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_dsrc_MapData(tvb, offset, &asn1_ctx, tree, hf_dsrc_dsrc_MapData_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/* --- Module AddGrpC --- --- ---                                             */


static const value_string AddGrpC_TimeReference_vals[] = {
  {   1, "oneMilliSec" },
  { 0, NULL }
};


static int
dissect_AddGrpC_TimeReference(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t AddGrpC_ItsStationPosition_sequence[] = {
  { &hf_AddGrpC_stationID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_StationID },
  { &hf_AddGrpC_laneID      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_LaneID },
  { &hf_AddGrpC_nodeXY      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_NodeOffsetPointXY },
  { &hf_AddGrpC_timeReference, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_AddGrpC_TimeReference },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_ItsStationPosition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_ItsStationPosition, AddGrpC_ItsStationPosition_sequence);

  return offset;
}


static const per_sequence_t AddGrpC_ItsStationPositionList_sequence_of[1] = {
  { &hf_AddGrpC_ItsStationPositionList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_AddGrpC_ItsStationPosition },
};

static int
dissect_AddGrpC_ItsStationPositionList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_AddGrpC_ItsStationPositionList, AddGrpC_ItsStationPositionList_sequence_of,
                                                  1, 5, FALSE);

  return offset;
}


static const per_sequence_t AddGrpC_ConnectionManeuverAssist_addGrpC_sequence[] = {
  { &hf_AddGrpC_itsStationPositions, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_AddGrpC_ItsStationPositionList },
  { &hf_AddGrpC_rsuGNSSOffset, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_NodeOffsetPointXY },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_ConnectionManeuverAssist_addGrpC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 341 "./asn1/its/its.cnf"
  actx->private_data = wmem_new0(wmem_packet_scope(), its_private_data_t);

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_ConnectionManeuverAssist_addGrpC, AddGrpC_ConnectionManeuverAssist_addGrpC_sequence);

  return offset;
}


static const per_sequence_t AddGrpC_ConnectionTrajectory_addGrpC_sequence[] = {
  { &hf_AddGrpC_nodes       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_NodeSetXY },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_ConnectionTrajectory_addGrpC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 346 "./asn1/its/its.cnf"
  actx->private_data = wmem_new0(wmem_packet_scope(), its_private_data_t);

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_ConnectionTrajectory_addGrpC, AddGrpC_ConnectionTrajectory_addGrpC_sequence);

  return offset;
}


static const value_string AddGrpC_PtvRequestType_vals[] = {
  {   0, "preRequest" },
  {   1, "mainRequest" },
  {   2, "doorCloseRequest" },
  {   3, "cancelRequest" },
  {   4, "emergencyRequest" },
  { 0, NULL }
};


static int
dissect_AddGrpC_PtvRequestType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     5, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t AddGrpC_Control_addGrpC_sequence[] = {
  { &hf_AddGrpC_ptvRequest  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_AddGrpC_PtvRequestType },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_Control_addGrpC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 351 "./asn1/its/its.cnf"
  actx->private_data = wmem_new0(wmem_packet_scope(), its_private_data_t);

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_Control_addGrpC, AddGrpC_Control_addGrpC_sequence);

  return offset;
}


static const per_sequence_t AddGrpC_PrioritizationResponse_sequence[] = {
  { &hf_AddGrpC_stationID   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_StationID },
  { &hf_AddGrpC_priorState  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_PrioritizationResponseStatus },
  { &hf_AddGrpC_signalGroup , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_SignalGroupID },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_PrioritizationResponse(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_PrioritizationResponse, AddGrpC_PrioritizationResponse_sequence);

  return offset;
}


static const per_sequence_t AddGrpC_PrioritizationResponseList_sequence_of[1] = {
  { &hf_AddGrpC_PrioritizationResponseList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_AddGrpC_PrioritizationResponse },
};

static int
dissect_AddGrpC_PrioritizationResponseList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_AddGrpC_PrioritizationResponseList, AddGrpC_PrioritizationResponseList_sequence_of,
                                                  1, 10, FALSE);

  return offset;
}


static const per_sequence_t AddGrpC_IntersectionState_addGrpC_sequence[] = {
  { &hf_AddGrpC_activePrioritizations, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_AddGrpC_PrioritizationResponseList },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_IntersectionState_addGrpC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 356 "./asn1/its/its.cnf"
  actx->private_data = wmem_new0(wmem_packet_scope(), its_private_data_t);

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_IntersectionState_addGrpC, AddGrpC_IntersectionState_addGrpC_sequence);

  return offset;
}


static const per_sequence_t AddGrpC_SignalHeadLocation_sequence[] = {
  { &hf_AddGrpC_nodeXY      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_NodeOffsetPointXY },
  { &hf_AddGrpC_nodeZ       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_DeltaAltitude },
  { &hf_AddGrpC_signalGroupID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_dsrc_SignalGroupID },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_SignalHeadLocation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_SignalHeadLocation, AddGrpC_SignalHeadLocation_sequence);

  return offset;
}


static const per_sequence_t AddGrpC_SignalHeadLocationList_sequence_of[1] = {
  { &hf_AddGrpC_SignalHeadLocationList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_AddGrpC_SignalHeadLocation },
};

static int
dissect_AddGrpC_SignalHeadLocationList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_AddGrpC_SignalHeadLocationList, AddGrpC_SignalHeadLocationList_sequence_of,
                                                  1, 64, FALSE);

  return offset;
}


static const per_sequence_t AddGrpC_MapData_addGrpC_sequence[] = {
  { &hf_AddGrpC_signalHeadLocations, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_AddGrpC_SignalHeadLocationList },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_MapData_addGrpC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 361 "./asn1/its/its.cnf"
  actx->private_data = wmem_new0(wmem_packet_scope(), its_private_data_t);

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_MapData_addGrpC, AddGrpC_MapData_addGrpC_sequence);

  return offset;
}


static const per_sequence_t AddGrpC_Position3D_addGrpC_sequence[] = {
  { &hf_AddGrpC_altitude    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Altitude },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_Position3D_addGrpC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 366 "./asn1/its/its.cnf"
  actx->private_data = wmem_new0(wmem_packet_scope(), its_private_data_t);

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_Position3D_addGrpC, AddGrpC_Position3D_addGrpC_sequence);

  return offset;
}


static const value_string AddGrpC_EmissionType_vals[] = {
  {   0, "euro1" },
  {   1, "euro2" },
  {   2, "euro3" },
  {   3, "euro4" },
  {   4, "euro5" },
  {   5, "euro6" },
  { 0, NULL }
};


static int
dissect_AddGrpC_EmissionType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     6, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t AddGrpC_RestrictionUserType_addGrpC_sequence[] = {
  { &hf_AddGrpC_emission    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_AddGrpC_EmissionType },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_RestrictionUserType_addGrpC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 371 "./asn1/its/its.cnf"
  actx->private_data = wmem_new0(wmem_packet_scope(), its_private_data_t);

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_RestrictionUserType_addGrpC, AddGrpC_RestrictionUserType_addGrpC_sequence);

  return offset;
}


static const per_sequence_t AddGrpC_SignalStatusPackage_addGrpC_sequence[] = {
  { &hf_AddGrpC_synchToSchedule, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_dsrc_DeltaTime },
  { NULL, 0, 0, NULL }
};

static int
dissect_AddGrpC_SignalStatusPackage_addGrpC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 376 "./asn1/its/its.cnf"
  actx->private_data = wmem_new0(wmem_packet_scope(), its_private_data_t);

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_AddGrpC_SignalStatusPackage_addGrpC, AddGrpC_SignalStatusPackage_addGrpC_sequence);

  return offset;
}

/*--- PDUs ---*/

static int dissect_AddGrpC_ConnectionManeuverAssist_addGrpC_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_AddGrpC_ConnectionManeuverAssist_addGrpC(tvb, offset, &asn1_ctx, tree, hf_AddGrpC_AddGrpC_ConnectionManeuverAssist_addGrpC_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_AddGrpC_ConnectionTrajectory_addGrpC_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_AddGrpC_ConnectionTrajectory_addGrpC(tvb, offset, &asn1_ctx, tree, hf_AddGrpC_AddGrpC_ConnectionTrajectory_addGrpC_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_AddGrpC_Control_addGrpC_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_AddGrpC_Control_addGrpC(tvb, offset, &asn1_ctx, tree, hf_AddGrpC_AddGrpC_Control_addGrpC_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_AddGrpC_IntersectionState_addGrpC_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_AddGrpC_IntersectionState_addGrpC(tvb, offset, &asn1_ctx, tree, hf_AddGrpC_AddGrpC_IntersectionState_addGrpC_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_AddGrpC_MapData_addGrpC_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_AddGrpC_MapData_addGrpC(tvb, offset, &asn1_ctx, tree, hf_AddGrpC_AddGrpC_MapData_addGrpC_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_AddGrpC_Position3D_addGrpC_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_AddGrpC_Position3D_addGrpC(tvb, offset, &asn1_ctx, tree, hf_AddGrpC_AddGrpC_Position3D_addGrpC_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_AddGrpC_RestrictionUserType_addGrpC_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_AddGrpC_RestrictionUserType_addGrpC(tvb, offset, &asn1_ctx, tree, hf_AddGrpC_AddGrpC_RestrictionUserType_addGrpC_PDU);
  offset += 7; offset >>= 3;
  return offset;
}
static int dissect_AddGrpC_SignalStatusPackage_addGrpC_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_AddGrpC_SignalStatusPackage_addGrpC(tvb, offset, &asn1_ctx, tree, hf_AddGrpC_AddGrpC_SignalStatusPackage_addGrpC_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/* --- Module REGION --- --- ---                                              */


/* --- Module SPATEM-PDU-Descriptions --- --- ---                             */


/* --- Module MAPEM-PDU-Descriptions --- --- ---                              */


/* --- Module IVIM-PDU-Descriptions --- --- ---                               */


/* --- Module SREM-PDU-Descriptions --- --- ---                               */


/* --- Module SSEM-PDU-Descriptions --- --- ---                               */


/* --- Module CAM-PDU-Descriptions --- --- ---                                */


static const value_string cam_GenerationDeltaTime_vals[] = {
  {   1, "oneMilliSec" },
  { 0, NULL }
};


static int
dissect_cam_GenerationDeltaTime(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t cam_BasicContainer_sequence[] = {
  { &hf_cam_stationType     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_StationType },
  { &hf_cam_referencePosition, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_ReferencePosition },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_BasicContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_BasicContainer, cam_BasicContainer_sequence);

  return offset;
}


static const per_sequence_t cam_BasicVehicleContainerHighFrequency_sequence[] = {
  { &hf_cam_heading         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Heading },
  { &hf_cam_speed           , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Speed },
  { &hf_cam_driveDirection  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DriveDirection },
  { &hf_cam_vehicleLength   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_VehicleLength },
  { &hf_cam_vehicleWidth    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_VehicleWidth },
  { &hf_cam_longitudinalAcceleration, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_LongitudinalAcceleration },
  { &hf_cam_curvature       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_Curvature },
  { &hf_cam_curvatureCalculationMode, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_CurvatureCalculationMode },
  { &hf_cam_yawRate         , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_YawRate },
  { &hf_cam_accelerationControl, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_AccelerationControl },
  { &hf_cam_lanePosition    , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_LanePosition },
  { &hf_cam_steeringWheelAngle, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_SteeringWheelAngle },
  { &hf_cam_lateralAcceleration, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_LateralAcceleration },
  { &hf_cam_verticalAcceleration, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_VerticalAcceleration },
  { &hf_cam_performanceClass, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_PerformanceClass },
  { &hf_cam_cenDsrcTollingZone, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_CenDsrcTollingZone },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_BasicVehicleContainerHighFrequency(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_BasicVehicleContainerHighFrequency, cam_BasicVehicleContainerHighFrequency_sequence);

  return offset;
}


static const per_sequence_t cam_RSUContainerHighFrequency_sequence[] = {
  { &hf_cam_protectedCommunicationZonesRSU, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_ProtectedCommunicationZonesRSU },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_RSUContainerHighFrequency(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_RSUContainerHighFrequency, cam_RSUContainerHighFrequency_sequence);

  return offset;
}


static const value_string cam_HighFrequencyContainer_vals[] = {
  {   0, "basicVehicleContainerHighFrequency" },
  {   1, "rsuContainerHighFrequency" },
  { 0, NULL }
};

static const per_choice_t cam_HighFrequencyContainer_choice[] = {
  {   0, &hf_cam_basicVehicleContainerHighFrequency, ASN1_EXTENSION_ROOT    , dissect_cam_BasicVehicleContainerHighFrequency },
  {   1, &hf_cam_rsuContainerHighFrequency, ASN1_EXTENSION_ROOT    , dissect_cam_RSUContainerHighFrequency },
  { 0, NULL, 0, NULL }
};

static int
dissect_cam_HighFrequencyContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_cam_HighFrequencyContainer, cam_HighFrequencyContainer_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t cam_BasicVehicleContainerLowFrequency_sequence[] = {
  { &hf_cam_vehicleRole     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_VehicleRole },
  { &hf_cam_exteriorLights  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_ExteriorLights },
  { &hf_cam_pathHistory     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PathHistory },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_BasicVehicleContainerLowFrequency(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_BasicVehicleContainerLowFrequency, cam_BasicVehicleContainerLowFrequency_sequence);

  return offset;
}


static const value_string cam_LowFrequencyContainer_vals[] = {
  {   0, "basicVehicleContainerLowFrequency" },
  { 0, NULL }
};

static const per_choice_t cam_LowFrequencyContainer_choice[] = {
  {   0, &hf_cam_basicVehicleContainerLowFrequency, ASN1_EXTENSION_ROOT    , dissect_cam_BasicVehicleContainerLowFrequency },
  { 0, NULL, 0, NULL }
};

static int
dissect_cam_LowFrequencyContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_cam_LowFrequencyContainer, cam_LowFrequencyContainer_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t cam_PublicTransportContainer_sequence[] = {
  { &hf_cam_embarkationStatus, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_EmbarkationStatus },
  { &hf_cam_ptActivation    , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_PtActivation },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_PublicTransportContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_PublicTransportContainer, cam_PublicTransportContainer_sequence);

  return offset;
}


static const per_sequence_t cam_SpecialTransportContainer_sequence[] = {
  { &hf_cam_specialTransportType, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_SpecialTransportType },
  { &hf_cam_lightBarSirenInUse, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_LightBarSirenInUse },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_SpecialTransportContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_SpecialTransportContainer, cam_SpecialTransportContainer_sequence);

  return offset;
}


static const per_sequence_t cam_DangerousGoodsContainer_sequence[] = {
  { &hf_cam_dangerousGoodsBasic, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_DangerousGoodsBasic },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_DangerousGoodsContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_DangerousGoodsContainer, cam_DangerousGoodsContainer_sequence);

  return offset;
}


static const per_sequence_t cam_RoadWorksContainerBasic_sequence[] = {
  { &hf_cam_roadworksSubCauseCode, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_RoadworksSubCauseCode },
  { &hf_cam_lightBarSirenInUse, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_LightBarSirenInUse },
  { &hf_cam_closedLanes     , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_ClosedLanes },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_RoadWorksContainerBasic(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_RoadWorksContainerBasic, cam_RoadWorksContainerBasic_sequence);

  return offset;
}


static const per_sequence_t cam_RescueContainer_sequence[] = {
  { &hf_cam_lightBarSirenInUse, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_LightBarSirenInUse },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_RescueContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_RescueContainer, cam_RescueContainer_sequence);

  return offset;
}


static const per_sequence_t cam_EmergencyContainer_sequence[] = {
  { &hf_cam_lightBarSirenInUse, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_LightBarSirenInUse },
  { &hf_cam_incidentIndication, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_CauseCode },
  { &hf_cam_emergencyPriority, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_EmergencyPriority },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_EmergencyContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_EmergencyContainer, cam_EmergencyContainer_sequence);

  return offset;
}


static const per_sequence_t cam_SafetyCarContainer_sequence[] = {
  { &hf_cam_lightBarSirenInUse, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_LightBarSirenInUse },
  { &hf_cam_incidentIndication, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_CauseCode },
  { &hf_cam_trafficRule     , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_TrafficRule },
  { &hf_cam_speedLimit      , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_SpeedLimit },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_SafetyCarContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_SafetyCarContainer, cam_SafetyCarContainer_sequence);

  return offset;
}


static const value_string cam_SpecialVehicleContainer_vals[] = {
  {   0, "publicTransportContainer" },
  {   1, "specialTransportContainer" },
  {   2, "dangerousGoodsContainer" },
  {   3, "roadWorksContainerBasic" },
  {   4, "rescueContainer" },
  {   5, "emergencyContainer" },
  {   6, "safetyCarContainer" },
  { 0, NULL }
};

static const per_choice_t cam_SpecialVehicleContainer_choice[] = {
  {   0, &hf_cam_publicTransportContainer, ASN1_EXTENSION_ROOT    , dissect_cam_PublicTransportContainer },
  {   1, &hf_cam_specialTransportContainer, ASN1_EXTENSION_ROOT    , dissect_cam_SpecialTransportContainer },
  {   2, &hf_cam_dangerousGoodsContainer, ASN1_EXTENSION_ROOT    , dissect_cam_DangerousGoodsContainer },
  {   3, &hf_cam_roadWorksContainerBasic, ASN1_EXTENSION_ROOT    , dissect_cam_RoadWorksContainerBasic },
  {   4, &hf_cam_rescueContainer , ASN1_EXTENSION_ROOT    , dissect_cam_RescueContainer },
  {   5, &hf_cam_emergencyContainer, ASN1_EXTENSION_ROOT    , dissect_cam_EmergencyContainer },
  {   6, &hf_cam_safetyCarContainer, ASN1_EXTENSION_ROOT    , dissect_cam_SafetyCarContainer },
  { 0, NULL, 0, NULL }
};

static int
dissect_cam_SpecialVehicleContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_cam_SpecialVehicleContainer, cam_SpecialVehicleContainer_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t cam_CamParameters_sequence[] = {
  { &hf_cam_basicContainer  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_cam_BasicContainer },
  { &hf_cam_highFrequencyContainer, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_cam_HighFrequencyContainer },
  { &hf_cam_lowFrequencyContainer, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_cam_LowFrequencyContainer },
  { &hf_cam_specialVehicleContainer, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_cam_SpecialVehicleContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_CamParameters(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_CamParameters, cam_CamParameters_sequence);

  return offset;
}


static const per_sequence_t cam_CoopAwareness_sequence[] = {
  { &hf_cam_generationDeltaTime, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_cam_GenerationDeltaTime },
  { &hf_cam_camParameters   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_cam_CamParameters },
  { NULL, 0, 0, NULL }
};

static int
dissect_cam_CoopAwareness(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 272 "./asn1/its/its.cnf"
  actx->private_data = (void*)wmem_new0(wmem_packet_scope(), its_private_data_t);
  col_set_str(actx->pinfo->cinfo, COL_PROTOCOL, "CAM");
  col_set_str(actx->pinfo->cinfo, COL_INFO, "CAM");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_cam_CoopAwareness, cam_CoopAwareness_sequence);

  return offset;
}

/*--- PDUs ---*/

static int dissect_cam_CoopAwareness_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_cam_CoopAwareness(tvb, offset, &asn1_ctx, tree, hf_cam_cam_CoopAwareness_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/* --- Module DENM-PDU-Descriptions --- --- ---                               */


static const value_string denm_Termination_vals[] = {
  {   0, "isCancellation" },
  {   1, "isNegation" },
  { 0, NULL }
};


static int
dissect_denm_Termination(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t denm_ManagementContainer_sequence[] = {
  { &hf_denm_actionID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_ActionID },
  { &hf_denm_detectionTime  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_denm_referenceTime  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_denm_termination    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_denm_Termination },
  { &hf_denm_eventPosition  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_ReferencePosition },
  { &hf_denm_relevanceDistance, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_RelevanceDistance },
  { &hf_denm_relevanceTrafficDirection, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_RelevanceTrafficDirection },
  { &hf_denm_validityDuration, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_ValidityDuration },
  { &hf_denm_transmissionInterval, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_TransmissionInterval },
  { &hf_denm_stationType    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_StationType },
  { NULL, 0, 0, NULL }
};

static int
dissect_denm_ManagementContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_denm_ManagementContainer, denm_ManagementContainer_sequence);

  return offset;
}


static const per_sequence_t denm_SituationContainer_sequence[] = {
  { &hf_denm_informationQuality, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_InformationQuality },
  { &hf_denm_eventType      , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_CauseCode },
  { &hf_denm_linkedCause    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_CauseCode },
  { &hf_denm_eventHistory   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_EventHistory },
  { NULL, 0, 0, NULL }
};

static int
dissect_denm_SituationContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_denm_SituationContainer, denm_SituationContainer_sequence);

  return offset;
}


static const per_sequence_t denm_LocationContainer_sequence[] = {
  { &hf_denm_eventSpeed     , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_Speed },
  { &hf_denm_eventPositionHeading, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_Heading },
  { &hf_denm_traces         , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Traces },
  { &hf_denm_roadType       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_RoadType },
  { NULL, 0, 0, NULL }
};

static int
dissect_denm_LocationContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_denm_LocationContainer, denm_LocationContainer_sequence);

  return offset;
}


static const per_sequence_t denm_ImpactReductionContainer_sequence[] = {
  { &hf_denm_heightLonCarrLeft, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_HeightLonCarr },
  { &hf_denm_heightLonCarrRight, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_HeightLonCarr },
  { &hf_denm_posLonCarrLeft , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PosLonCarr },
  { &hf_denm_posLonCarrRight, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PosLonCarr },
  { &hf_denm_positionOfPillars, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PositionOfPillars },
  { &hf_denm_posCentMass    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PosCentMass },
  { &hf_denm_wheelBaseVehicle, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_WheelBaseVehicle },
  { &hf_denm_turningRadius  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_TurningRadius },
  { &hf_denm_posFrontAx     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PosFrontAx },
  { &hf_denm_positionOfOccupants, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_PositionOfOccupants },
  { &hf_denm_vehicleMass    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_VehicleMass },
  { &hf_denm_requestResponseIndication, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_RequestResponseIndication },
  { NULL, 0, 0, NULL }
};

static int
dissect_denm_ImpactReductionContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_denm_ImpactReductionContainer, denm_ImpactReductionContainer_sequence);

  return offset;
}


static const per_sequence_t denm_ReferenceDenms_sequence_of[1] = {
  { &hf_denm_ReferenceDenms_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_ActionID },
};

static int
dissect_denm_ReferenceDenms(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_denm_ReferenceDenms, denm_ReferenceDenms_sequence_of,
                                                  1, 8, TRUE);

  return offset;
}


static const per_sequence_t denm_RoadWorksContainerExtended_sequence[] = {
  { &hf_denm_lightBarSirenInUse, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_LightBarSirenInUse },
  { &hf_denm_closedLanes    , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_ClosedLanes },
  { &hf_denm_restriction    , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_RestrictedTypes },
  { &hf_denm_speedLimit     , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_SpeedLimit },
  { &hf_denm_incidentIndication, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_CauseCode },
  { &hf_denm_recommendedPath, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_ItineraryPath },
  { &hf_denm_startingPointSpeedLimit, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_DeltaReferencePosition },
  { &hf_denm_trafficFlowRule, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_TrafficRule },
  { &hf_denm_referenceDenms , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_denm_ReferenceDenms },
  { NULL, 0, 0, NULL }
};

static int
dissect_denm_RoadWorksContainerExtended(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_denm_RoadWorksContainerExtended, denm_RoadWorksContainerExtended_sequence);

  return offset;
}


static const per_sequence_t denm_StationaryVehicleContainer_sequence[] = {
  { &hf_denm_stationarySince, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_StationarySince },
  { &hf_denm_stationaryCause, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_CauseCode },
  { &hf_denm_carryingDangerousGoods, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_DangerousGoodsExtended },
  { &hf_denm_numberOfOccupants, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_NumberOfOccupants },
  { &hf_denm_vehicleIdentification, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_VehicleIdentification },
  { &hf_denm_energyStorageType, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_its_EnergyStorageType },
  { NULL, 0, 0, NULL }
};

static int
dissect_denm_StationaryVehicleContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_denm_StationaryVehicleContainer, denm_StationaryVehicleContainer_sequence);

  return offset;
}


static const per_sequence_t denm_AlacarteContainer_sequence[] = {
  { &hf_denm_lanePosition   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_LanePosition },
  { &hf_denm_impactReduction, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_denm_ImpactReductionContainer },
  { &hf_denm_externalTemperature, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_Temperature },
  { &hf_denm_roadWorks      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_denm_RoadWorksContainerExtended },
  { &hf_denm_positioningSolution, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_PositioningSolutionType },
  { &hf_denm_stationaryVehicle, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_denm_StationaryVehicleContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_denm_AlacarteContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_denm_AlacarteContainer, denm_AlacarteContainer_sequence);

  return offset;
}


static const per_sequence_t denm_DecentralizedEnvironmentalNotificationMessage_sequence[] = {
  { &hf_denm_management     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_denm_ManagementContainer },
  { &hf_denm_situation      , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_denm_SituationContainer },
  { &hf_denm_location       , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_denm_LocationContainer },
  { &hf_denm_alacarte       , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_denm_AlacarteContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_denm_DecentralizedEnvironmentalNotificationMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 278 "./asn1/its/its.cnf"
  actx->private_data = (void*)wmem_new0(wmem_packet_scope(), its_private_data_t);
  col_set_str(actx->pinfo->cinfo, COL_PROTOCOL, "DENM");
  col_set_str(actx->pinfo->cinfo, COL_INFO, "DENM");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_denm_DecentralizedEnvironmentalNotificationMessage, denm_DecentralizedEnvironmentalNotificationMessage_sequence);

  return offset;
}

/*--- PDUs ---*/

static int dissect_denm_DecentralizedEnvironmentalNotificationMessage_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_denm_DecentralizedEnvironmentalNotificationMessage(tvb, offset, &asn1_ctx, tree, hf_denm_denm_DecentralizedEnvironmentalNotificationMessage_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/* --- Module TIS-TPG-Transactions-Descriptions --- --- ---                   */


static const value_string tistpg_UNVehicleClassifcation_vals[] = {
  {   0, "reserved" },
  {   1, "categoryL1" },
  {   2, "categoryL2" },
  {   3, "categoryL3" },
  {   4, "categoryL4" },
  {   5, "categoryL5" },
  {   6, "categoryL6" },
  {   7, "categoryL7" },
  {   8, "categoryL8" },
  { 0, NULL }
};


static int
dissect_tistpg_UNVehicleClassifcation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 63U, NULL, FALSE);

  return offset;
}



static int
dissect_tistpg_CustomerContract(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_IA5String(tvb, offset, actx, tree, hf_index,
                                          1, 32, FALSE);

  return offset;
}



static int
dissect_tistpg_TisProfile(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 724 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     8, 8, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_tistpg_TisProfile_reserved,
      &hf_tistpg_TisProfile_profileOne,
      &hf_tistpg_TisProfile_profileTwo,
      &hf_tistpg_TisProfile_profileThree,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_tistpg_TisProfile);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const per_sequence_t tistpg_TisTpgDRM_Management_sequence[] = {
  { &hf_tistpg_generationTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_tistpg_vehicleType  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_UNVehicleClassifcation },
  { &hf_tistpg_costumerContract, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_CustomerContract },
  { &hf_tistpg_tisProfile   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TisProfile },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgDRM_Management(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgDRM_Management, tistpg_TisTpgDRM_Management_sequence);

  return offset;
}


static const per_sequence_t tistpg_TisTpgDRM_Situation_sequence[] = {
  { &hf_tistpg_causeCode    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_CauseCode },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgDRM_Situation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgDRM_Situation, tistpg_TisTpgDRM_Situation_sequence);

  return offset;
}



static int
dissect_tistpg_SearchRange(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const value_string tistpg_SearchCondition_vals[] = {
  {   0, "nearest" },
  {   1, "quickest" },
  {   2, "paylessRoad" },
  { 0, NULL }
};


static int
dissect_tistpg_SearchCondition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}


static const per_sequence_t tistpg_TisTpgDRM_Location_sequence[] = {
  { &hf_tistpg_vehiclePosition, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_ReferencePosition },
  { &hf_tistpg_vehicleSpeed , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Speed },
  { &hf_tistpg_vehicleHeading, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Heading },
  { &hf_tistpg_requestedPosition, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_ReferencePosition },
  { &hf_tistpg_searchRange  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_SearchRange },
  { &hf_tistpg_searchCondition, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_SearchCondition },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgDRM_Location(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgDRM_Location, tistpg_TisTpgDRM_Location_sequence);

  return offset;
}


static const per_sequence_t tistpg_TisTpgDRM_sequence[] = {
  { &hf_tistpg_drmManagement, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TisTpgDRM_Management },
  { &hf_tistpg_drmSituation , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TisTpgDRM_Situation },
  { &hf_tistpg_drmLocation  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TisTpgDRM_Location },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgDRM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgDRM, tistpg_TisTpgDRM_sequence);

  return offset;
}



static int
dissect_tistpg_TotalTpgStations(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t tistpg_TisTpgSNM_Management_sequence[] = {
  { &hf_tistpg_generationTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_tistpg_totalTpgStations, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TotalTpgStations },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgSNM_Management(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgSNM_Management, tistpg_TisTpgSNM_Management_sequence);

  return offset;
}



static int
dissect_tistpg_TpgAutomation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 666 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     4, 4, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_tistpg_TpgAutomation_fullAutomated,
      &hf_tistpg_TpgAutomation_semiAutomated,
      &hf_tistpg_TpgAutomation_manual,
      &hf_tistpg_TpgAutomation_reserved,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_tistpg_TpgAutomation);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_tistpg_TpgNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 65535U, NULL, FALSE);

  return offset;
}



static int
dissect_tistpg_TpgProvider(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 32, FALSE);

  return offset;
}



static int
dissect_tistpg_Accessibility(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 32, FALSE);

  return offset;
}



static int
dissect_tistpg_UTF8String_SIZE_1_128(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 128, FALSE);

  return offset;
}



static int
dissect_tistpg_BookingInfo(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          NO_BOUND, NO_BOUND, FALSE);

  return offset;
}



static int
dissect_tistpg_AvailableTpgNumber(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 65535U, NULL, FALSE);

  return offset;
}



static int
dissect_tistpg_CancellationCondition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 32, FALSE);

  return offset;
}


static const per_sequence_t tistpg_TpgStationData_sequence[] = {
  { &hf_tistpg_tpgStationID , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_StationID },
  { &hf_tistpg_tpgAutomationLevel, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TpgAutomation },
  { &hf_tistpg_tpgNumber    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TpgNumber },
  { &hf_tistpg_tpgProvider  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TpgProvider },
  { &hf_tistpg_tpgLocation  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_ReferencePosition },
  { &hf_tistpg_accessibility, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_Accessibility },
  { &hf_tistpg_address      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_UTF8String_SIZE_1_128 },
  { &hf_tistpg_phoneNumber  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_PhoneNumber },
  { &hf_tistpg_digitalMap   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_DigitalMap },
  { &hf_tistpg_openingDaysHours, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_OpeningDaysHours },
  { &hf_tistpg_bookingInfo  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_BookingInfo },
  { &hf_tistpg_availableTpgNumber, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_AvailableTpgNumber },
  { &hf_tistpg_cancellationCondition, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_CancellationCondition },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TpgStationData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TpgStationData, tistpg_TpgStationData_sequence);

  return offset;
}


static const per_sequence_t tistpg_TpgNotifContainer_sequence_of[1] = {
  { &hf_tistpg_TpgNotifContainer_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_TpgStationData },
};

static int
dissect_tistpg_TpgNotifContainer(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_tistpg_TpgNotifContainer, tistpg_TpgNotifContainer_sequence_of,
                                                  1, 8, FALSE);

  return offset;
}


static const per_sequence_t tistpg_TisTpgSNM_sequence[] = {
  { &hf_tistpg_snmManagement, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_TisTpgSNM_Management },
  { &hf_tistpg_tpgContainer , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_TpgNotifContainer },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgSNM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgSNM, tistpg_TisTpgSNM_sequence);

  return offset;
}


static const value_string tistpg_ReservationStatus_vals[] = {
  {   0, "reservationOK" },
  {   1, "noReservationService" },
  {   2, "noTpmsAvailable" },
  { 0, NULL }
};


static int
dissect_tistpg_ReservationStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}



static int
dissect_tistpg_ReservationID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 32, FALSE);

  return offset;
}


static const per_sequence_t tistpg_TisTpgTRM_Management_sequence[] = {
  { &hf_tistpg_generationTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_tistpg_vehicleType  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_UNVehicleClassifcation },
  { &hf_tistpg_tpgStationID , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_StationID },
  { &hf_tistpg_reservationStatus, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_ReservationStatus },
  { &hf_tistpg_costumercontract, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_CustomerContract },
  { &hf_tistpg_reservationID, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_ReservationID },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgTRM_Management(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgTRM_Management, tistpg_TisTpgTRM_Management_sequence);

  return offset;
}



static int
dissect_tistpg_PairingID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 9999U, NULL, FALSE);

  return offset;
}


static const per_sequence_t tistpg_TisTpgTRM_Situation_sequence[] = {
  { &hf_tistpg_estArrivalTime, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_tistpg_proposedPairingID, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_tistpg_PairingID },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgTRM_Situation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgTRM_Situation, tistpg_TisTpgTRM_Situation_sequence);

  return offset;
}


static const per_sequence_t tistpg_TisTpgTRM_Location_sequence[] = {
  { &hf_tistpg_vehiclePosition, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_ReferencePosition },
  { &hf_tistpg_vehicleSpeed , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Speed },
  { &hf_tistpg_vehicleHeading, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_Heading },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgTRM_Location(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgTRM_Location, tistpg_TisTpgTRM_Location_sequence);

  return offset;
}


static const per_sequence_t tistpg_TisTpgTRM_sequence[] = {
  { &hf_tistpg_trmManagement, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TisTpgTRM_Management },
  { &hf_tistpg_trmSituation , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_TisTpgTRM_Situation },
  { &hf_tistpg_trmLocation  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_TisTpgTRM_Location },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgTRM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgTRM, tistpg_TisTpgTRM_sequence);

  return offset;
}



static int
dissect_tistpg_INTEGER_1_65535(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 65535U, NULL, FALSE);

  return offset;
}


static const per_sequence_t tistpg_TisTpgTCM_Management_sequence[] = {
  { &hf_tistpg_generationTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_tistpg_tpgStationID , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_StationID },
  { &hf_tistpg_reservationStatus, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_ReservationStatus },
  { &hf_tistpg_reservedTpg  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_INTEGER_1_65535 },
  { &hf_tistpg_costumercontract, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_CustomerContract },
  { &hf_tistpg_reservationID, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_ReservationID },
  { &hf_tistpg_tpgAutomationLevel, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_TpgAutomation },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgTCM_Management(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgTCM_Management, tistpg_TisTpgTCM_Management_sequence);

  return offset;
}


static const per_sequence_t tistpg_TisTpgTCM_Situation_sequence[] = {
  { &hf_tistpg_pairingID    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_PairingID },
  { &hf_tistpg_reservationTimeLimit, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_tistpg_cancellationCondition, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_CancellationCondition },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgTCM_Situation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgTCM_Situation, tistpg_TisTpgTCM_Situation_sequence);

  return offset;
}


static const per_sequence_t tistpg_TisTpgTCM_Location_sequence[] = {
  { &hf_tistpg_tpgLocation  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_ReferencePosition },
  { &hf_tistpg_address      , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_UTF8String_SIZE_1_128 },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgTCM_Location(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgTCM_Location, tistpg_TisTpgTCM_Location_sequence);

  return offset;
}


static const per_sequence_t tistpg_TisTpgTCM_sequence[] = {
  { &hf_tistpg_tcmManagement, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TisTpgTCM_Management },
  { &hf_tistpg_tcmSituation , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_TisTpgTCM_Situation },
  { &hf_tistpg_tcmLocation  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_TisTpgTCM_Location },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgTCM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgTCM, tistpg_TisTpgTCM_sequence);

  return offset;
}


static const value_string tistpg_FillingStatus_vals[] = {
  {   0, "requestVehicleData" },
  {   1, "sendVehicleData" },
  {   2, "started" },
  {   3, "fillingProcessFailed" },
  {   4, "fillingProcessCompleted" },
  { 0, NULL }
};


static int
dissect_tistpg_FillingStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}


static const per_sequence_t tistpg_TisTpgVDRM_Management_sequence[] = {
  { &hf_tistpg_generationTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_tistpg_fillingStatus, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_FillingStatus },
  { &hf_tistpg_automationLevel, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TpgAutomation },
  { &hf_tistpg_pairingID    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_PairingID },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgVDRM_Management(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgVDRM_Management, tistpg_TisTpgVDRM_Management_sequence);

  return offset;
}


static const per_sequence_t tistpg_TisTpgVDRM_sequence[] = {
  { &hf_tistpg_vdrmManagement, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TisTpgVDRM_Management },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgVDRM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgVDRM, tistpg_TisTpgVDRM_sequence);

  return offset;
}



static int
dissect_tistpg_Language(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     10, 10, FALSE, NULL, NULL);

  return offset;
}


static const value_string tistpg_TyreTempCondition_vals[] = {
  {   0, "pressure-cold" },
  {   1, "pressure-warm" },
  {   2, "unavailable" },
  { 0, NULL }
};


static int
dissect_tistpg_TyreTempCondition(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, TRUE, 0, NULL);

  return offset;
}


static const per_sequence_t tistpg_TisTpgVDPM_Management_sequence[] = {
  { &hf_tistpg_generationTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_tistpg_tisProfile   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_TisProfile },
  { &hf_tistpg_language     , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_Language },
  { &hf_tistpg_vehicleType  , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_UNVehicleClassifcation },
  { &hf_tistpg_tyreTempCondition, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TyreTempCondition },
  { &hf_tistpg_fillingStatus, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_FillingStatus },
  { &hf_tistpg_pairingID    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_PairingID },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgVDPM_Management(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgVDPM_Management, tistpg_TisTpgVDPM_Management_sequence);

  return offset;
}



static int
dissect_tistpg_TyreSetVariantID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 15U, NULL, FALSE);

  return offset;
}



static int
dissect_tistpg_TyreSidewallInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     60, 60, FALSE, NULL, NULL);

  return offset;
}



static int
dissect_tistpg_PressureConfiguration(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     9, 9, FALSE, NULL, NULL);

  return offset;
}


static const value_string tistpg_AxlePlacardPressure_vals[] = {
  {   0, "zero" },
  {   1, "fiveKPa" },
  { 0, NULL }
};


static int
dissect_tistpg_AxlePlacardPressure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const per_sequence_t tistpg_PressureVariant_sequence[] = {
  { &hf_tistpg_pressureConfiguration, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_PressureConfiguration },
  { &hf_tistpg_frontAxlePressure, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_AxlePlacardPressure },
  { &hf_tistpg_rearAxlePressure, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_AxlePlacardPressure },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_PressureVariant(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_PressureVariant, tistpg_PressureVariant_sequence);

  return offset;
}


static const per_sequence_t tistpg_PressureVariantsList_sequence_of[1] = {
  { &hf_tistpg_PressureVariantsList_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_PressureVariant },
};

static int
dissect_tistpg_PressureVariantsList(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_tistpg_PressureVariantsList, tistpg_PressureVariantsList_sequence_of,
                                                  1, 15, FALSE);

  return offset;
}


static const per_sequence_t tistpg_TyreSetVariant_sequence[] = {
  { &hf_tistpg_variantID    , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_TyreSetVariantID },
  { &hf_tistpg_frontAxleDimension, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_tistpg_TyreSidewallInformation },
  { &hf_tistpg_rearAxleDimension, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_tistpg_TyreSidewallInformation },
  { &hf_tistpg_pressureVariantsList, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_PressureVariantsList },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TyreSetVariant(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TyreSetVariant, tistpg_TyreSetVariant_sequence);

  return offset;
}


static const per_sequence_t tistpg_PlacardTable_sequence_of[1] = {
  { &hf_tistpg_PlacardTable_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_TyreSetVariant },
};

static int
dissect_tistpg_PlacardTable(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_tistpg_PlacardTable, tistpg_PlacardTable_sequence_of,
                                                  0, 15, FALSE);

  return offset;
}


static const value_string tistpg_TyrePressure_vals[] = {
  {   0, "invalid" },
  {   1, "lessThanOneBar" },
  {   2, "oneBar" },
  {   3, "oneBarPlusTwoAndHalfKPa" },
  { 254, "inflation" },
  { 255, "overflow" },
  { 0, NULL }
};


static int
dissect_tistpg_TyrePressure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}



static int
dissect_tistpg_NULL(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_null(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const value_string tistpg_T_currentTyrePressure_vals[] = {
  {   0, "tyrePressureValue" },
  {   1, "unavailable" },
  { 0, NULL }
};

static const per_choice_t tistpg_T_currentTyrePressure_choice[] = {
  {   0, &hf_tistpg_tyrePressureValue, ASN1_NO_EXTENSIONS     , dissect_tistpg_TyrePressure },
  {   1, &hf_tistpg_unavailable  , ASN1_NO_EXTENSIONS     , dissect_tistpg_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_tistpg_T_currentTyrePressure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_tistpg_T_currentTyrePressure, tistpg_T_currentTyrePressure_choice,
                                 NULL);

  return offset;
}


static const value_string tistpg_T_tyreSidewallInformation_vals[] = {
  {   0, "tyreSidewallInformationValue" },
  {   1, "unavailable" },
  { 0, NULL }
};

static const per_choice_t tistpg_T_tyreSidewallInformation_choice[] = {
  {   0, &hf_tistpg_tyreSidewallInformationValue, ASN1_NO_EXTENSIONS     , dissect_tistpg_TyreSidewallInformation },
  {   1, &hf_tistpg_unavailable  , ASN1_NO_EXTENSIONS     , dissect_tistpg_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_tistpg_T_tyreSidewallInformation(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_tistpg_T_tyreSidewallInformation, tistpg_T_tyreSidewallInformation_choice,
                                 NULL);

  return offset;
}


static const value_string tistpg_TyreAirTemperature_vals[] = {
  {   0, "invalid" },
  {   1, "lessThanMinus50Celsius" },
  {   2, "minus50Celsius" },
  {   3, "minus49Celsius" },
  {  52, "zeroCelsius" },
  { 240, "overflowThreshold1" },
  { 241, "overflowThreshold2" },
  { 242, "overflowThreshold3" },
  { 243, "overflowThreshold4" },
  { 244, "overflowThreshold5" },
  { 245, "overflowThreshold6" },
  { 246, "overflowThreshold7" },
  { 247, "overflowThreshold8" },
  { 248, "overflowThreshold9" },
  { 249, "overflowThreshold10" },
  { 250, "overflowThreshold11" },
  { 251, "overflowThreshold12" },
  { 252, "overflowThreshold13" },
  { 253, "overflowThreshold14" },
  { 254, "overflowThreshold15" },
  { 255, "overflowThreshold16" },
  { 0, NULL }
};


static int
dissect_tistpg_TyreAirTemperature(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 255U, NULL, FALSE);

  return offset;
}


static const value_string tistpg_T_currentInsideAirTemperature_vals[] = {
  {   0, "tyreAirTemperatureValue" },
  {   1, "unavailable" },
  { 0, NULL }
};

static const per_choice_t tistpg_T_currentInsideAirTemperature_choice[] = {
  {   0, &hf_tistpg_tyreAirTemperatureValue, ASN1_NO_EXTENSIONS     , dissect_tistpg_TyreAirTemperature },
  {   1, &hf_tistpg_unavailable  , ASN1_NO_EXTENSIONS     , dissect_tistpg_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_tistpg_T_currentInsideAirTemperature(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_tistpg_T_currentInsideAirTemperature, tistpg_T_currentInsideAirTemperature_choice,
                                 NULL);

  return offset;
}


static const value_string tistpg_T_recommendedTyrePressure_vals[] = {
  {   0, "axlePlacardPressureValue" },
  {   1, "unavailable" },
  { 0, NULL }
};

static const per_choice_t tistpg_T_recommendedTyrePressure_choice[] = {
  {   0, &hf_tistpg_axlePlacardPressureValue, ASN1_NO_EXTENSIONS     , dissect_tistpg_AxlePlacardPressure },
  {   1, &hf_tistpg_unavailable  , ASN1_NO_EXTENSIONS     , dissect_tistpg_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_tistpg_T_recommendedTyrePressure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_tistpg_T_recommendedTyrePressure, tistpg_T_recommendedTyrePressure_choice,
                                 NULL);

  return offset;
}



static int
dissect_tistpg_TIN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     64, 64, FALSE, NULL, NULL);

  return offset;
}


static const value_string tistpg_T_tin_vals[] = {
  {   0, "tinValue" },
  {   1, "unavailable" },
  { 0, NULL }
};

static const per_choice_t tistpg_T_tin_choice[] = {
  {   0, &hf_tistpg_tinValue     , ASN1_NO_EXTENSIONS     , dissect_tistpg_TIN },
  {   1, &hf_tistpg_unavailable  , ASN1_NO_EXTENSIONS     , dissect_tistpg_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_tistpg_T_tin(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_tistpg_T_tin, tistpg_T_tin_choice,
                                 NULL);

  return offset;
}


static const value_string tistpg_SensorState_vals[] = {
  { 65534, "malfunction" },
  { 65535, "unavailable" },
  { 0, NULL }
};


static int
dissect_tistpg_SensorState(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}


static const value_string tistpg_T_sensorState_vals[] = {
  {   0, "sensorStateValue" },
  {   1, "unavailable" },
  { 0, NULL }
};

static const per_choice_t tistpg_T_sensorState_choice[] = {
  {   0, &hf_tistpg_sensorStateValue, ASN1_NO_EXTENSIONS     , dissect_tistpg_SensorState },
  {   1, &hf_tistpg_unavailable  , ASN1_NO_EXTENSIONS     , dissect_tistpg_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_tistpg_T_sensorState(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_tistpg_T_sensorState, tistpg_T_sensorState_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t tistpg_TyreData_sequence[] = {
  { &hf_tistpg_currentTyrePressure, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_T_currentTyrePressure },
  { &hf_tistpg_tyreSidewallInformation, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_T_tyreSidewallInformation },
  { &hf_tistpg_currentInsideAirTemperature, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_T_currentInsideAirTemperature },
  { &hf_tistpg_recommendedTyrePressure, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_T_recommendedTyrePressure },
  { &hf_tistpg_tin          , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_T_tin },
  { &hf_tistpg_sensorState  , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_T_sensorState },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TyreData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TyreData, tistpg_TyreData_sequence);

  return offset;
}


static const per_sequence_t tistpg_VehicleSpecificData_sequence[] = {
  { &hf_tistpg_currentVehicleConfiguration, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_PressureConfiguration },
  { &hf_tistpg_frontLeftTyreData, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TyreData },
  { &hf_tistpg_frontRightTyreData, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TyreData },
  { &hf_tistpg_rearLeftTyreData, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TyreData },
  { &hf_tistpg_rearRightTyreData, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TyreData },
  { &hf_tistpg_spareTyreData, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TyreData },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_VehicleSpecificData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_VehicleSpecificData, tistpg_VehicleSpecificData_sequence);

  return offset;
}


static const per_sequence_t tistpg_TisTpgVDPM_sequence[] = {
  { &hf_tistpg_vdpmManagement, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TisTpgVDPM_Management },
  { &hf_tistpg_placardTable , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_PlacardTable },
  { &hf_tistpg_vehicleSpecificData, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_VehicleSpecificData },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgVDPM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgVDPM, tistpg_TisTpgVDPM_sequence);

  return offset;
}


static const value_string tistpg_NumberOfAppliedPressure_vals[] = {
  {   1, "oneAppliedPressure" },
  {   2, "twoAppliedPressure" },
  { 0, NULL }
};


static int
dissect_tistpg_NumberOfAppliedPressure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 5U, NULL, FALSE);

  return offset;
}


static const value_string tistpg_AppliedTyrePressure_vals[] = {
  {   0, "tyrePressureValue" },
  {   1, "unavailable" },
  { 0, NULL }
};

static const per_choice_t tistpg_AppliedTyrePressure_choice[] = {
  {   0, &hf_tistpg_tyrePressureValue, ASN1_NO_EXTENSIONS     , dissect_tistpg_TyrePressure },
  {   1, &hf_tistpg_unavailable  , ASN1_NO_EXTENSIONS     , dissect_tistpg_NULL },
  { 0, NULL, 0, NULL }
};

static int
dissect_tistpg_AppliedTyrePressure(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_tistpg_AppliedTyrePressure, tistpg_AppliedTyrePressure_choice,
                                 NULL);

  return offset;
}


static const per_sequence_t tistpg_AppliedTyrePressures_sequence_of[1] = {
  { &hf_tistpg_AppliedTyrePressures_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_tistpg_AppliedTyrePressure },
};

static int
dissect_tistpg_AppliedTyrePressures(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_tistpg_AppliedTyrePressures, tistpg_AppliedTyrePressures_sequence_of,
                                                  1, 5, FALSE);

  return offset;
}


static const per_sequence_t tistpg_TisTpgEOFM_Management_sequence[] = {
  { &hf_tistpg_generationTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_tistpg_fillingStatus, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_FillingStatus },
  { &hf_tistpg_numberOfAppliedPressure, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_NumberOfAppliedPressure },
  { &hf_tistpg_appliedTyrePressures, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_AppliedTyrePressures },
  { &hf_tistpg_pairingID    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_tistpg_PairingID },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgEOFM_Management(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgEOFM_Management, tistpg_TisTpgEOFM_Management_sequence);

  return offset;
}


static const per_sequence_t tistpg_TisTpgEOFM_sequence[] = {
  { &hf_tistpg_eofmManagement, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_tistpg_TisTpgEOFM_Management },
  { NULL, 0, 0, NULL }
};

static int
dissect_tistpg_TisTpgEOFM(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_tistpg_TisTpgEOFM, tistpg_TisTpgEOFM_sequence);

  return offset;
}


static const value_string tistpg_TisTpgTransaction_vals[] = {
  {   0, "drm" },
  {   1, "snm" },
  {   2, "trm" },
  {   3, "tcm" },
  {   4, "vdrm" },
  {   5, "vdpm" },
  {   6, "eofm" },
  { 0, NULL }
};

static const per_choice_t tistpg_TisTpgTransaction_choice[] = {
  {   0, &hf_tistpg_drm          , ASN1_NO_EXTENSIONS     , dissect_tistpg_TisTpgDRM },
  {   1, &hf_tistpg_snm          , ASN1_NO_EXTENSIONS     , dissect_tistpg_TisTpgSNM },
  {   2, &hf_tistpg_trm          , ASN1_NO_EXTENSIONS     , dissect_tistpg_TisTpgTRM },
  {   3, &hf_tistpg_tcm          , ASN1_NO_EXTENSIONS     , dissect_tistpg_TisTpgTCM },
  {   4, &hf_tistpg_vdrm         , ASN1_NO_EXTENSIONS     , dissect_tistpg_TisTpgVDRM },
  {   5, &hf_tistpg_vdpm         , ASN1_NO_EXTENSIONS     , dissect_tistpg_TisTpgVDPM },
  {   6, &hf_tistpg_eofm         , ASN1_NO_EXTENSIONS     , dissect_tistpg_TisTpgEOFM },
  { 0, NULL, 0, NULL }
};

static int
dissect_tistpg_TisTpgTransaction(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 296 "./asn1/its/its.cnf"
  actx->private_data = (void*)wmem_new0(wmem_packet_scope(), its_private_data_t);
  col_set_str(actx->pinfo->cinfo, COL_PROTOCOL, "TISTPG");
  col_set_str(actx->pinfo->cinfo, COL_INFO, "TISTPG");

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_tistpg_TisTpgTransaction, tistpg_TisTpgTransaction_choice,
                                 NULL);

  return offset;
}

/*--- PDUs ---*/

static int dissect_tistpg_TisTpgTransaction_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_tistpg_TisTpgTransaction(tvb, offset, &asn1_ctx, tree, hf_tistpg_tistpg_TisTpgTransaction_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/* --- Module EVCSN-PDU-Descriptions --- --- ---                              */



static int
dissect_evcsn_POIType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 65535U, NULL, FALSE);

  return offset;
}



static int
dissect_evcsn_BOOLEAN(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_boolean(tvb, offset, actx, tree, hf_index, NULL);

  return offset;
}


static const per_sequence_t evcsn_ItsPOIHeader_sequence[] = {
  { &hf_evcsn_poiType       , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_POIType },
  { &hf_evcsn_timeStamp     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_its_TimestampIts },
  { &hf_evcsn_relayCapable  , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_BOOLEAN },
  { NULL, 0, 0, NULL }
};

static int
dissect_evcsn_ItsPOIHeader(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evcsn_ItsPOIHeader, evcsn_ItsPOIHeader_sequence);

  return offset;
}



static int
dissect_evcsn_NumberStations(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 256U, NULL, FALSE);

  return offset;
}



static int
dissect_evcsn_UTF8String_SIZE_1_32(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 32, FALSE);

  return offset;
}



static int
dissect_evcsn_UTF8String(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          NO_BOUND, NO_BOUND, FALSE);

  return offset;
}



static int
dissect_evcsn_NumericString_SIZE_1_16(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_NumericString(tvb, offset, actx, tree, hf_index,
                                          1, 16, FALSE);

  return offset;
}



static int
dissect_evcsn_ChargingSpotType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 616 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     NO_BOUND, NO_BOUND, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields0[] = { // CHECKED
      &hf_evcsn_ChargingSpotType_standardChargeMode1,
      &hf_evcsn_ChargingSpotType_standardChargeMode2,
      &hf_evcsn_ChargingSpotType_standardOrFastChargeMode3,
      &hf_evcsn_ChargingSpotType_fastChargeWithExternalCharger,
      NULL
    };
    const gint *fields1[] = { // CHECKED
      &hf_evcsn_ChargingSpotType_quickDrop,
      &hf_evcsn_ChargingSpotType_inductiveChargeWhileStationary,
      &hf_evcsn_ChargingSpotType_inductiveChargeWhileDriving,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_evcsn_ChargingSpotType);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields0, ENC_BIG_ENDIAN);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 1, 1, fields1, ENC_BIG_ENDIAN);
  }



  return offset;
}



static int
dissect_evcsn_TypeOfReceptacle(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 1113 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  int len;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     NO_BOUND, NO_BOUND, FALSE, &parameter_tvb, &len);

  /* TODO Provide values?
   * See ETSI TS 101 556-1 V1.1.1 Annex B Description for data elements
   * Table B.2: Coding of the type of receptacle field
   *
   * Code  Charging     Standard plug /      Type of  Nb of   Maximum  Maximum
   *       spot type    cable                current  phases  Voltage  Current
   * -------------------------------------------------------------------------
   * 0000  Socket       IEC 62196-2 type 1     AC     Single   240V     16A
   * 0001  Socket       IEC 62196-2 type 1     AC     Single   240V     32A
   * 0010  Socket       IEC 62196-2 type 1     AC     Single   240V     80A
   * 0011  Socket       IEC 62196-2 type 1     AC     Single   120V     12A
   * 0100  Socket       IEC 62196-2 type 1     AC     Single   120V     16A
   * 0101  Socket       Standard Household     AC     Single   250V     16A
   * 0110  Socket       Standard Household     AC     Single   480V     16A
   * 0111  Socket       IEC 62196-2 type 2     AC     Single   230V     16A
   * 1000  Socket       IEC 62196-2 type 2     AC     Single   250V     32A
   * 1001  Socket       IEC 62196-2 type 2     AC     Single   480V     32A
   * 1010  Socket       IEC 62196-2 type 2     AC     Single   400V     32/250A
   * 1011  Socket       IEC 62196-2 type 3A    AC     Single   250V     32A
   * 1100  Socket       IEC 62196-2 type 3B    AC     Single   480V     32A
   * 1101  Socket       Reserved for Future    AC     Single  1000V     400A
                        IEC 62196-3
   * 1110  Cable for    Reserved
           DC charging
   * 1111  Cable for    Reserved
           DC charging
   */



  return offset;
}



static int
dissect_evcsn_INTEGER_0_1400(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 1400U, NULL, FALSE);

  return offset;
}


static const per_sequence_t evcsn_SpotAvailability_sequence[] = {
  { &hf_evcsn_maxWaitingTimeMinutes, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_INTEGER_0_1400 },
  { &hf_evcsn_blocking      , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_BOOLEAN },
  { NULL, 0, 0, NULL }
};

static int
dissect_evcsn_SpotAvailability(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evcsn_SpotAvailability, evcsn_SpotAvailability_sequence);

  return offset;
}


static const per_sequence_t evcsn_ParkingPlacesData_sequence_of[1] = {
  { &hf_evcsn_ParkingPlacesData_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_SpotAvailability },
};

static int
dissect_evcsn_ParkingPlacesData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_evcsn_ParkingPlacesData, evcsn_ParkingPlacesData_sequence_of,
                                                  1, 4, FALSE);

  return offset;
}


static const per_sequence_t evcsn_ItsChargingSpotDataElements_sequence[] = {
  { &hf_evcsn_type          , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_ChargingSpotType },
  { &hf_evcsn_evEquipmentID , ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_evcsn_UTF8String },
  { &hf_evcsn_typeOfReceptacle, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_TypeOfReceptacle },
  { &hf_evcsn_energyAvailability, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_UTF8String },
  { &hf_evcsn_parkingPlacesData, ASN1_NO_EXTENSIONS     , ASN1_OPTIONAL    , dissect_evcsn_ParkingPlacesData },
  { NULL, 0, 0, NULL }
};

static int
dissect_evcsn_ItsChargingSpotDataElements(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evcsn_ItsChargingSpotDataElements, evcsn_ItsChargingSpotDataElements_sequence);

  return offset;
}


static const per_sequence_t evcsn_ItsChargingSpots_sequence_of[1] = {
  { &hf_evcsn_ItsChargingSpots_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_ItsChargingSpotDataElements },
};

static int
dissect_evcsn_ItsChargingSpots(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_evcsn_ItsChargingSpots, evcsn_ItsChargingSpots_sequence_of,
                                                  1, 16, FALSE);

  return offset;
}


static const per_sequence_t evcsn_ItsChargingStationData_sequence[] = {
  { &hf_evcsn_chargingStationID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_StationID },
  { &hf_evcsn_utilityDistributorId, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evcsn_UTF8String_SIZE_1_32 },
  { &hf_evcsn_providerID    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evcsn_UTF8String_SIZE_1_32 },
  { &hf_evcsn_chargingStationLocation, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_its_ReferencePosition },
  { &hf_evcsn_address       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evcsn_UTF8String },
  { &hf_evcsn_phoneNumber   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evcsn_NumericString_SIZE_1_16 },
  { &hf_evcsn_accessibility , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evcsn_UTF8String_SIZE_1_32 },
  { &hf_evcsn_digitalMap    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_its_DigitalMap },
  { &hf_evcsn_openingDaysHours, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evcsn_UTF8String },
  { &hf_evcsn_pricing       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evcsn_UTF8String },
  { &hf_evcsn_bookingContactInfo, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evcsn_UTF8String },
  { &hf_evcsn_payment       , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evcsn_UTF8String },
  { &hf_evcsn_chargingSpotsAvailable, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evcsn_ItsChargingSpots },
  { NULL, 0, 0, NULL }
};

static int
dissect_evcsn_ItsChargingStationData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evcsn_ItsChargingStationData, evcsn_ItsChargingStationData_sequence);

  return offset;
}


static const per_sequence_t evcsn_SEQUENCE_SIZE_1_256_OF_ItsChargingStationData_sequence_of[1] = {
  { &hf_evcsn_chargingStationsData_item, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_ItsChargingStationData },
};

static int
dissect_evcsn_SEQUENCE_SIZE_1_256_OF_ItsChargingStationData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_sequence_of(tvb, offset, actx, tree, hf_index,
                                                  ett_evcsn_SEQUENCE_SIZE_1_256_OF_ItsChargingStationData, evcsn_SEQUENCE_SIZE_1_256_OF_ItsChargingStationData_sequence_of,
                                                  1, 256, FALSE);

  return offset;
}


static const per_sequence_t evcsn_ItsEVCSNData_sequence[] = {
  { &hf_evcsn_totalNumberOfStations, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_NumberStations },
  { &hf_evcsn_chargingStationsData, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_SEQUENCE_SIZE_1_256_OF_ItsChargingStationData },
  { NULL, 0, 0, NULL }
};

static int
dissect_evcsn_ItsEVCSNData(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evcsn_ItsEVCSNData, evcsn_ItsEVCSNData_sequence);

  return offset;
}


static const per_sequence_t evcsn_EVChargingSpotNotificationPOIMessage_sequence[] = {
  { &hf_evcsn_poiHeader     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_ItsPOIHeader },
  { &hf_evcsn_evcsnData     , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evcsn_ItsEVCSNData },
  { NULL, 0, 0, NULL }
};

static int
dissect_evcsn_EVChargingSpotNotificationPOIMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 284 "./asn1/its/its.cnf"
  actx->private_data = (void*)wmem_new0(wmem_packet_scope(), its_private_data_t);
  col_set_str(actx->pinfo->cinfo, COL_PROTOCOL, "EVCSN");
  col_set_str(actx->pinfo->cinfo, COL_INFO, "EVCSN");

  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evcsn_EVChargingSpotNotificationPOIMessage, evcsn_EVChargingSpotNotificationPOIMessage_sequence);

  return offset;
}

/*--- PDUs ---*/

static int dissect_evcsn_EVChargingSpotNotificationPOIMessage_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_evcsn_EVChargingSpotNotificationPOIMessage(tvb, offset, &asn1_ctx, tree, hf_evcsn_evcsn_EVChargingSpotNotificationPOIMessage_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/* --- Module EV-RechargingSpotReservation-PDU-Descriptions --- --- ---       */



static int
dissect_evrsr_EVSE_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_octet_string(tvb, offset, actx, tree, hf_index,
                                       1, 32, FALSE, NULL);

  return offset;
}


static const value_string evrsr_TimestampUTC_vals[] = {
  {   0, "utcStartOf2013" },
  {   1, "oneSecondAfterUTCStartOf2013" },
  { 0, NULL }
};


static int
dissect_evrsr_TimestampUTC(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_integer(tvb, offset, actx, tree, hf_index, NULL);

  return offset;
}


static const value_string evrsr_RechargingMode_vals[] = {
  {   0, "mode1" },
  {   1, "mode2" },
  {   2, "mode3" },
  {   3, "mode4" },
  {   8, "quickDrop" },
  {  12, "inductiveChargingWhileStationary" },
  {  14, "inductiveChargingWhileDriving" },
  { 0, NULL }
};


static int
dissect_evrsr_RechargingMode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 15U, NULL, FALSE);

  return offset;
}


static const value_string evrsr_PowerSource_vals[] = {
  {   0, "notApplicable" },
  {   1, "ac1Phase" },
  {   2, "ac2Phase" },
  {   3, "ac3Phase" },
  {   4, "dcc" },
  {   5, "chaDeMo" },
  { 0, NULL }
};


static int
dissect_evrsr_PowerSource(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            0U, 7U, NULL, FALSE);

  return offset;
}


static const per_sequence_t evrsr_RechargingType_sequence[] = {
  { &hf_evrsr_rechargingMode, ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evrsr_RechargingMode },
  { &hf_evrsr_powerSource   , ASN1_NO_EXTENSIONS     , ASN1_NOT_OPTIONAL, dissect_evrsr_PowerSource },
  { NULL, 0, 0, NULL }
};

static int
dissect_evrsr_RechargingType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evrsr_RechargingType, evrsr_RechargingType_sequence);

  return offset;
}



static int
dissect_evrsr_BatteryType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 16, FALSE);

  return offset;
}


static const per_sequence_t evrsr_PreReservationRequestMessage_sequence[] = {
  { &hf_evrsr_evse_ID       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_EVSE_ID },
  { &hf_evrsr_arrivalTime   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_TimestampUTC },
  { &hf_evrsr_departureTime , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_TimestampUTC },
  { &hf_evrsr_rechargingType, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_RechargingType },
  { &hf_evrsr_batteryType   , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_BatteryType },
  { NULL, 0, 0, NULL }
};

static int
dissect_evrsr_PreReservationRequestMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evrsr_PreReservationRequestMessage, evrsr_PreReservationRequestMessage_sequence);

  return offset;
}



static int
dissect_evrsr_Reservation_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_VisibleString(tvb, offset, actx, tree, hf_index,
                                          8, 8, FALSE);

  return offset;
}



static int
dissect_evrsr_PreReservation_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_evrsr_Reservation_ID(tvb, offset, actx, tree, hf_index);

  return offset;
}


static const value_string evrsr_AvailabilityStatus_vals[] = {
  {   0, "available" },
  {   1, "no-free-capacity" },
  { 0, NULL }
};


static int
dissect_evrsr_AvailabilityStatus(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_evrsr_SupportedPaymentTypes(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 638 "./asn1/its/its.cnf"
  tvbuff_t *parameter_tvb = NULL;
  offset = dissect_per_bit_string(tvb, offset, actx, tree, hf_index,
                                     2, 2, FALSE, &parameter_tvb, NULL);

  if (parameter_tvb) {
    const gint *fields[] = { // CHECKED
      &hf_evrsr_SupportedPaymentTypes_contract,
      &hf_evrsr_SupportedPaymentTypes_externalIdentification,
      NULL
    };
    proto_tree *subtree = proto_item_add_subtree(actx->created_item, ett_evrsr_SupportedPaymentTypes);
    proto_tree_add_bitmask_list(subtree, parameter_tvb, 0, 1, fields, ENC_BIG_ENDIAN);
  }



  return offset;
}


static const per_sequence_t evrsr_PreReservationResponseMessage_sequence[] = {
  { &hf_evrsr_preReservation_ID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_PreReservation_ID },
  { &hf_evrsr_availabilityStatus, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_AvailabilityStatus },
  { &hf_evrsr_preReservationExpirationTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_TimestampUTC },
  { &hf_evrsr_supportedPaymentTypes, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_SupportedPaymentTypes },
  { NULL, 0, 0, NULL }
};

static int
dissect_evrsr_PreReservationResponseMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evrsr_PreReservationResponseMessage, evrsr_PreReservationResponseMessage_sequence);

  return offset;
}


static const value_string evrsr_EAmount_vals[] = {
  {   1, "oneWh" },
  { 0, NULL }
};


static int
dissect_evrsr_EAmount(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_constrained_integer(tvb, offset, actx, tree, hf_index,
                                                            1U, 500000U, NULL, FALSE);

  return offset;
}


static const value_string evrsr_PaymentType_vals[] = {
  {   0, "contract" },
  {   1, "externalIdentification" },
  { 0, NULL }
};


static int
dissect_evrsr_PaymentType(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     2, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_evrsr_ContractID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 24, FALSE);

  return offset;
}



static int
dissect_evrsr_ExternalIdentificationMeans(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 24, FALSE);

  return offset;
}


static const value_string evrsr_Payment_ID_vals[] = {
  {   0, "contractID" },
  {   1, "externalIdentificationMeans" },
  { 0, NULL }
};

static const per_choice_t evrsr_Payment_ID_choice[] = {
  {   0, &hf_evrsr_contractID    , ASN1_NO_EXTENSIONS     , dissect_evrsr_ContractID },
  {   1, &hf_evrsr_externalIdentificationMeans, ASN1_NO_EXTENSIONS     , dissect_evrsr_ExternalIdentificationMeans },
  { 0, NULL, 0, NULL }
};

static int
dissect_evrsr_Payment_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_evrsr_Payment_ID, evrsr_Payment_ID_choice,
                                 NULL);

  return offset;
}



static int
dissect_evrsr_Pairing_ID(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_VisibleString(tvb, offset, actx, tree, hf_index,
                                          1, 64, FALSE);

  return offset;
}


static const per_sequence_t evrsr_ReservationRequestMessage_sequence[] = {
  { &hf_evrsr_currentTime   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_TimestampUTC },
  { &hf_evrsr_preReservation_ID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_PreReservation_ID },
  { &hf_evrsr_arrivalTime   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_TimestampUTC },
  { &hf_evrsr_departureTime , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_TimestampUTC },
  { &hf_evrsr_eAmount       , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_EAmount },
  { &hf_evrsr_eAmountMin    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_EAmount },
  { &hf_evrsr_paymentType   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_PaymentType },
  { &hf_evrsr_payment_ID    , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_Payment_ID },
  { &hf_evrsr_secondPayment_ID, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_Payment_ID },
  { &hf_evrsr_pairing_ID    , ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_Pairing_ID },
  { NULL, 0, 0, NULL }
};

static int
dissect_evrsr_ReservationRequestMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evrsr_ReservationRequestMessage, evrsr_ReservationRequestMessage_sequence);

  return offset;
}


static const value_string evrsr_ReservationResponseCode_vals[] = {
  {   0, "ok" },
  {   1, "invalid-EVSE-ID" },
  {   2, "payment-type-not-supported" },
  {   3, "payment-error" },
  {   4, "authentication-error" },
  {   5, "insufficient-power-availability" },
  { 0, NULL }
};


static int
dissect_evrsr_ReservationResponseCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     6, NULL, FALSE, 0, NULL);

  return offset;
}



static int
dissect_evrsr_Reservation_Password(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_VisibleString(tvb, offset, actx, tree, hf_index,
                                          8, 8, FALSE);

  return offset;
}



static int
dissect_evrsr_StationDetails(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 32, FALSE);

  return offset;
}



static int
dissect_evrsr_ChargingSpotLabel(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_UTF8String(tvb, offset, actx, tree, hf_index,
                                          1, 4, FALSE);

  return offset;
}


static const per_sequence_t evrsr_ReservationResponseMessage_sequence[] = {
  { &hf_evrsr_reservationResponseCode, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_ReservationResponseCode },
  { &hf_evrsr_reservation_ID, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_Reservation_ID },
  { &hf_evrsr_reservation_Password, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_Reservation_Password },
  { &hf_evrsr_stationDetails, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_StationDetails },
  { &hf_evrsr_chargingSpotLabel, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_ChargingSpotLabel },
  { &hf_evrsr_expirationTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_TimestampUTC },
  { &hf_evrsr_freeCancelTimeLimit, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_TimestampUTC },
  { NULL, 0, 0, NULL }
};

static int
dissect_evrsr_ReservationResponseMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evrsr_ReservationResponseMessage, evrsr_ReservationResponseMessage_sequence);

  return offset;
}


static const per_sequence_t evrsr_CancellationRequestMessage_sequence[] = {
  { &hf_evrsr_reservation_ID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_Reservation_ID },
  { &hf_evrsr_reservation_Password, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_Reservation_Password },
  { &hf_evrsr_currentTime   , ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_TimestampUTC },
  { NULL, 0, 0, NULL }
};

static int
dissect_evrsr_CancellationRequestMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evrsr_CancellationRequestMessage, evrsr_CancellationRequestMessage_sequence);

  return offset;
}


static const value_string evrsr_CancellationResponseCode_vals[] = {
  {   0, "ok" },
  {   1, "unknown-Reservation-ID" },
  {   2, "mismatching-Reservation-Password" },
  { 0, NULL }
};


static int
dissect_evrsr_CancellationResponseCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     3, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t evrsr_CancellationResponseMessage_sequence[] = {
  { &hf_evrsr_reservation_ID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_Reservation_ID },
  { &hf_evrsr_cancellationResponseCode, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_CancellationResponseCode },
  { NULL, 0, 0, NULL }
};

static int
dissect_evrsr_CancellationResponseMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evrsr_CancellationResponseMessage, evrsr_CancellationResponseMessage_sequence);

  return offset;
}


static const per_sequence_t evrsr_UpdateRequestMessage_sequence[] = {
  { &hf_evrsr_reservation_ID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_Reservation_ID },
  { &hf_evrsr_reservation_Password, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_Reservation_Password },
  { &hf_evrsr_updatedArrivalTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_TimestampUTC },
  { &hf_evrsr_updatedDepartureTime, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_TimestampUTC },
  { NULL, 0, 0, NULL }
};

static int
dissect_evrsr_UpdateRequestMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evrsr_UpdateRequestMessage, evrsr_UpdateRequestMessage_sequence);

  return offset;
}


static const value_string evrsr_UpdateResponseCode_vals[] = {
  {   0, "ok" },
  {   1, "unknown-Reservation-ID" },
  {   2, "mismatching-Reservation-Password" },
  {   3, "invalid-Arrival-Time" },
  {   4, "invalid-Departure-Time" },
  { 0, NULL }
};


static int
dissect_evrsr_UpdateResponseCode(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_enumerated(tvb, offset, actx, tree, hf_index,
                                     5, NULL, FALSE, 0, NULL);

  return offset;
}


static const per_sequence_t evrsr_UpdateResponseMessage_sequence[] = {
  { &hf_evrsr_reservation_ID, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_Reservation_ID },
  { &hf_evrsr_updateResponseCode, ASN1_EXTENSION_ROOT    , ASN1_NOT_OPTIONAL, dissect_evrsr_UpdateResponseCode },
  { &hf_evrsr_chargingSpotLabel, ASN1_EXTENSION_ROOT    , ASN1_OPTIONAL    , dissect_evrsr_ChargingSpotLabel },
  { NULL, 0, 0, NULL }
};

static int
dissect_evrsr_UpdateResponseMessage(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_per_sequence(tvb, offset, actx, tree, hf_index,
                                   ett_evrsr_UpdateResponseMessage, evrsr_UpdateResponseMessage_sequence);

  return offset;
}


static const value_string evrsr_EV_RSR_MessageBody_vals[] = {
  {   0, "preReservationRequestMessage" },
  {   1, "preReservationResponseMessage" },
  {   2, "reservationRequestMessage" },
  {   3, "reservationResponseMessage" },
  {   4, "cancellationRequestMessage" },
  {   5, "cancellationResponseMessage" },
  {   6, "updateRequestMessage" },
  {   7, "updateResponseMessage" },
  { 0, NULL }
};

static const per_choice_t evrsr_EV_RSR_MessageBody_choice[] = {
  {   0, &hf_evrsr_preReservationRequestMessage, ASN1_EXTENSION_ROOT    , dissect_evrsr_PreReservationRequestMessage },
  {   1, &hf_evrsr_preReservationResponseMessage, ASN1_EXTENSION_ROOT    , dissect_evrsr_PreReservationResponseMessage },
  {   2, &hf_evrsr_reservationRequestMessage, ASN1_EXTENSION_ROOT    , dissect_evrsr_ReservationRequestMessage },
  {   3, &hf_evrsr_reservationResponseMessage, ASN1_EXTENSION_ROOT    , dissect_evrsr_ReservationResponseMessage },
  {   4, &hf_evrsr_cancellationRequestMessage, ASN1_EXTENSION_ROOT    , dissect_evrsr_CancellationRequestMessage },
  {   5, &hf_evrsr_cancellationResponseMessage, ASN1_EXTENSION_ROOT    , dissect_evrsr_CancellationResponseMessage },
  {   6, &hf_evrsr_updateRequestMessage, ASN1_EXTENSION_ROOT    , dissect_evrsr_UpdateRequestMessage },
  {   7, &hf_evrsr_updateResponseMessage, ASN1_EXTENSION_ROOT    , dissect_evrsr_UpdateResponseMessage },
  { 0, NULL, 0, NULL }
};

static int
dissect_evrsr_EV_RSR_MessageBody(tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 290 "./asn1/its/its.cnf"
  actx->private_data = (void*)wmem_new0(wmem_packet_scope(), its_private_data_t);
  col_set_str(actx->pinfo->cinfo, COL_PROTOCOL, "EV-RSR");
  col_set_str(actx->pinfo->cinfo, COL_INFO, "EV-RSR");

  offset = dissect_per_choice(tvb, offset, actx, tree, hf_index,
                                 ett_evrsr_EV_RSR_MessageBody, evrsr_EV_RSR_MessageBody_choice,
                                 NULL);

  return offset;
}

/*--- PDUs ---*/

static int dissect_evrsr_EV_RSR_MessageBody_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_) {
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_PER, FALSE, pinfo);
  offset = dissect_evrsr_EV_RSR_MessageBody(tvb, offset, &asn1_ctx, tree, hf_evrsr_evrsr_EV_RSR_MessageBody_PDU);
  offset += 7; offset >>= 3;
  return offset;
}


/*--- End of included file: packet-its-fn.c ---*/
#line 343 "./asn1/its/packet-its-template.c"

static int
dissect_its_PDU(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
  proto_item *its_item;
  proto_tree *its_tree;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "ITS");
  col_clear(pinfo->cinfo, COL_INFO);

  its_item = proto_tree_add_item(tree, proto_its, tvb, 0, -1, ENC_NA);
  its_tree = proto_item_add_subtree(its_item, ett_its);

  return dissect_its_ItsPduHeader_PDU(tvb, pinfo, its_tree, data);
}

// Decode As...
static void
its_msgid_prompt(packet_info *pinfo, gchar *result)
{
    guint32 msgid = GPOINTER_TO_UINT(p_get_proto_data(pinfo->pool, pinfo, hf_its_messageID, pinfo->curr_layer_num));

    g_snprintf(result, MAX_DECODE_AS_PROMPT_LEN, "MsgId (%s%u)", UTF8_RIGHTWARDS_ARROW, msgid);
}

static gpointer
its_msgid_value(packet_info *pinfo)
{
    return p_get_proto_data(pinfo->pool, pinfo, hf_its_messageID, pinfo->curr_layer_num);
}

// Registration of protocols
void proto_register_its(void)
{
    static hf_register_info hf_its[] = {

/*--- Included file: packet-its-hfarr.c ---*/
#line 1 "./asn1/its/packet-its-hfarr.c"

/* --- Module ITS-Container --- --- ---                                       */

    { &hf_its_its_ItsPduHeader_PDU,
      { "ItsPduHeader", "its.ItsPduHeader_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_protocolVersion,
      { "protocolVersion", "its.protocolVersion",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_its_messageID,
      { "messageID", "its.messageID",
        FT_UINT32, BASE_DEC, VALS(its_T_messageID_vals), 0,
        NULL, HFILL }},
    { &hf_its_stationID,
      { "stationID", "its.stationID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_its_latitude,
      { "latitude", "its.latitude",
        FT_INT32, BASE_DEC, VALS(its_Latitude_vals), 0,
        NULL, HFILL }},
    { &hf_its_longitude,
      { "longitude", "its.longitude",
        FT_INT32, BASE_DEC, VALS(its_Longitude_vals), 0,
        NULL, HFILL }},
    { &hf_its_positionConfidenceEllipse,
      { "positionConfidenceEllipse", "its.positionConfidenceEllipse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "PosConfidenceEllipse", HFILL }},
    { &hf_its_altitude,
      { "altitude", "its.altitude_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_deltaLatitude,
      { "deltaLatitude", "its.deltaLatitude",
        FT_INT32, BASE_DEC, VALS(its_DeltaLatitude_vals), 0,
        NULL, HFILL }},
    { &hf_its_deltaLongitude,
      { "deltaLongitude", "its.deltaLongitude",
        FT_INT32, BASE_DEC, VALS(its_DeltaLongitude_vals), 0,
        NULL, HFILL }},
    { &hf_its_deltaAltitude,
      { "deltaAltitude", "its.deltaAltitude",
        FT_INT32, BASE_DEC, VALS(its_DeltaAltitude_vals), 0,
        NULL, HFILL }},
    { &hf_its_altitudeValue,
      { "altitudeValue", "its.altitudeValue",
        FT_INT32, BASE_DEC, VALS(its_AltitudeValue_vals), 0,
        NULL, HFILL }},
    { &hf_its_altitudeConfidence,
      { "altitudeConfidence", "its.altitudeConfidence",
        FT_UINT32, BASE_DEC, VALS(its_AltitudeConfidence_vals), 0,
        NULL, HFILL }},
    { &hf_its_semiMajorConfidence,
      { "semiMajorConfidence", "its.semiMajorConfidence",
        FT_UINT32, BASE_DEC, VALS(its_SemiAxisLength_vals), 0,
        "SemiAxisLength", HFILL }},
    { &hf_its_semiMinorConfidence,
      { "semiMinorConfidence", "its.semiMinorConfidence",
        FT_UINT32, BASE_DEC, VALS(its_SemiAxisLength_vals), 0,
        "SemiAxisLength", HFILL }},
    { &hf_its_semiMajorOrientation,
      { "semiMajorOrientation", "its.semiMajorOrientation",
        FT_UINT32, BASE_DEC, VALS(its_HeadingValue_vals), 0,
        "HeadingValue", HFILL }},
    { &hf_its_pathPosition,
      { "pathPosition", "its.pathPosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "DeltaReferencePosition", HFILL }},
    { &hf_its_pathDeltaTime,
      { "pathDeltaTime", "its.pathDeltaTime",
        FT_UINT32, BASE_DEC, VALS(its_PathDeltaTime_vals), 0,
        NULL, HFILL }},
    { &hf_its_ptActivationType,
      { "ptActivationType", "its.ptActivationType",
        FT_UINT32, BASE_DEC, VALS(its_PtActivationType_vals), 0,
        NULL, HFILL }},
    { &hf_its_ptActivationData,
      { "ptActivationData", "its.ptActivationData",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_causeCode,
      { "causeCode", "its.causeCode",
        FT_UINT32, BASE_DEC, VALS(its_CauseCodeType_vals), 0,
        "CauseCodeType", HFILL }},
    { &hf_its_subCauseCode,
      { "subCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_curvatureValue,
      { "curvatureValue", "its.curvatureValue",
        FT_INT32, BASE_DEC, VALS(its_CurvatureValue_vals), 0,
        NULL, HFILL }},
    { &hf_its_curvatureConfidence,
      { "curvatureConfidence", "its.curvatureConfidence",
        FT_UINT32, BASE_DEC, VALS(its_CurvatureConfidence_vals), 0,
        NULL, HFILL }},
    { &hf_its_headingValue,
      { "headingValue", "its.headingValue",
        FT_UINT32, BASE_DEC, VALS(its_HeadingValue_vals), 0,
        NULL, HFILL }},
    { &hf_its_headingConfidence,
      { "headingConfidence", "its.headingConfidence",
        FT_UINT32, BASE_DEC, VALS(its_HeadingConfidence_vals), 0,
        NULL, HFILL }},
    { &hf_its_innerhardShoulderStatus,
      { "innerhardShoulderStatus", "its.innerhardShoulderStatus",
        FT_UINT32, BASE_DEC, VALS(its_HardShoulderStatus_vals), 0,
        "HardShoulderStatus", HFILL }},
    { &hf_its_outerhardShoulderStatus,
      { "outerhardShoulderStatus", "its.outerhardShoulderStatus",
        FT_UINT32, BASE_DEC, VALS(its_HardShoulderStatus_vals), 0,
        "HardShoulderStatus", HFILL }},
    { &hf_its_drivingLaneStatus,
      { "drivingLaneStatus", "its.drivingLaneStatus",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_speedValue,
      { "speedValue", "its.speedValue",
        FT_UINT32, BASE_DEC, VALS(its_SpeedValue_vals), 0,
        NULL, HFILL }},
    { &hf_its_speedConfidence,
      { "speedConfidence", "its.speedConfidence",
        FT_UINT32, BASE_DEC, VALS(its_SpeedConfidence_vals), 0,
        NULL, HFILL }},
    { &hf_its_longitudinalAccelerationValue,
      { "longitudinalAccelerationValue", "its.longitudinalAccelerationValue",
        FT_INT32, BASE_DEC, VALS(its_LongitudinalAccelerationValue_vals), 0,
        NULL, HFILL }},
    { &hf_its_longitudinalAccelerationConfidence,
      { "longitudinalAccelerationConfidence", "its.longitudinalAccelerationConfidence",
        FT_UINT32, BASE_DEC, VALS(its_AccelerationConfidence_vals), 0,
        "AccelerationConfidence", HFILL }},
    { &hf_its_lateralAccelerationValue,
      { "lateralAccelerationValue", "its.lateralAccelerationValue",
        FT_INT32, BASE_DEC, VALS(its_LateralAccelerationValue_vals), 0,
        NULL, HFILL }},
    { &hf_its_lateralAccelerationConfidence,
      { "lateralAccelerationConfidence", "its.lateralAccelerationConfidence",
        FT_UINT32, BASE_DEC, VALS(its_AccelerationConfidence_vals), 0,
        "AccelerationConfidence", HFILL }},
    { &hf_its_verticalAccelerationValue,
      { "verticalAccelerationValue", "its.verticalAccelerationValue",
        FT_INT32, BASE_DEC, VALS(its_VerticalAccelerationValue_vals), 0,
        NULL, HFILL }},
    { &hf_its_verticalAccelerationConfidence,
      { "verticalAccelerationConfidence", "its.verticalAccelerationConfidence",
        FT_UINT32, BASE_DEC, VALS(its_AccelerationConfidence_vals), 0,
        "AccelerationConfidence", HFILL }},
    { &hf_its_dangerousGoodsType,
      { "dangerousGoodsType", "its.dangerousGoodsType",
        FT_UINT32, BASE_DEC, VALS(its_DangerousGoodsBasic_vals), 0,
        "DangerousGoodsBasic", HFILL }},
    { &hf_its_unNumber,
      { "unNumber", "its.unNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_9999", HFILL }},
    { &hf_its_elevatedTemperature,
      { "elevatedTemperature", "its.elevatedTemperature",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_its_tunnelsRestricted,
      { "tunnelsRestricted", "its.tunnelsRestricted",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_its_limitedQuantity,
      { "limitedQuantity", "its.limitedQuantity",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_its_emergencyActionCode,
      { "emergencyActionCode", "its.emergencyActionCode",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_24", HFILL }},
    { &hf_its_phoneNumber,
      { "phoneNumber", "its.phoneNumber",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_companyName,
      { "companyName", "its.companyName",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String_SIZE_1_24", HFILL }},
    { &hf_its_wMInumber,
      { "wMInumber", "its.wMInumber",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_vDS,
      { "vDS", "its.vDS",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_vehicleLengthValue,
      { "vehicleLengthValue", "its.vehicleLengthValue",
        FT_UINT32, BASE_DEC, VALS(its_VehicleLengthValue_vals), 0,
        NULL, HFILL }},
    { &hf_its_vehicleLengthConfidenceIndication,
      { "vehicleLengthConfidenceIndication", "its.vehicleLengthConfidenceIndication",
        FT_UINT32, BASE_DEC, VALS(its_VehicleLengthConfidenceIndication_vals), 0,
        NULL, HFILL }},
    { &hf_its_PathHistory_item,
      { "PathPoint", "its.PathPoint_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_steeringWheelAngleValue,
      { "steeringWheelAngleValue", "its.steeringWheelAngleValue",
        FT_INT32, BASE_DEC, VALS(its_SteeringWheelAngleValue_vals), 0,
        NULL, HFILL }},
    { &hf_its_steeringWheelAngleConfidence,
      { "steeringWheelAngleConfidence", "its.steeringWheelAngleConfidence",
        FT_UINT32, BASE_DEC, VALS(its_SteeringWheelAngleConfidence_vals), 0,
        NULL, HFILL }},
    { &hf_its_yawRateValue,
      { "yawRateValue", "its.yawRateValue",
        FT_INT32, BASE_DEC, VALS(its_YawRateValue_vals), 0,
        NULL, HFILL }},
    { &hf_its_yawRateConfidence,
      { "yawRateConfidence", "its.yawRateConfidence",
        FT_UINT32, BASE_DEC, VALS(its_YawRateConfidence_vals), 0,
        NULL, HFILL }},
    { &hf_its_originatingStationID,
      { "originatingStationID", "its.originatingStationID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "StationID", HFILL }},
    { &hf_its_sequenceNumber,
      { "sequenceNumber", "its.sequenceNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_its_ItineraryPath_item,
      { "ReferencePosition", "its.ReferencePosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_protectedZoneType,
      { "protectedZoneType", "its.protectedZoneType",
        FT_UINT32, BASE_DEC, VALS(its_ProtectedZoneType_vals), 0,
        NULL, HFILL }},
    { &hf_its_expiryTime,
      { "expiryTime", "its.expiryTime",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_its_protectedZoneLatitude,
      { "protectedZoneLatitude", "its.protectedZoneLatitude",
        FT_INT32, BASE_DEC, VALS(its_Latitude_vals), 0,
        "Latitude", HFILL }},
    { &hf_its_protectedZoneLongitude,
      { "protectedZoneLongitude", "its.protectedZoneLongitude",
        FT_INT32, BASE_DEC, VALS(its_Longitude_vals), 0,
        "Longitude", HFILL }},
    { &hf_its_protectedZoneRadius,
      { "protectedZoneRadius", "its.protectedZoneRadius",
        FT_UINT32, BASE_DEC, VALS(its_ProtectedZoneRadius_vals), 0,
        NULL, HFILL }},
    { &hf_its_protectedZoneID,
      { "protectedZoneID", "its.protectedZoneID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_its_Traces_item,
      { "PathHistory", "its.PathHistory",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_its_PositionOfPillars_item,
      { "PosPillar", "its.PosPillar",
        FT_UINT32, BASE_DEC, VALS(its_PosPillar_vals), 0,
        NULL, HFILL }},
    { &hf_its_RestrictedTypes_item,
      { "StationType", "its.StationType",
        FT_UINT32, BASE_DEC, VALS(its_StationType_vals), 0,
        NULL, HFILL }},
    { &hf_its_EventHistory_item,
      { "EventPoint", "its.EventPoint_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_eventPosition,
      { "eventPosition", "its.eventPosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "DeltaReferencePosition", HFILL }},
    { &hf_its_eventDeltaTime,
      { "eventDeltaTime", "its.eventDeltaTime",
        FT_UINT32, BASE_DEC, VALS(its_PathDeltaTime_vals), 0,
        "PathDeltaTime", HFILL }},
    { &hf_its_informationQuality,
      { "informationQuality", "its.informationQuality",
        FT_UINT32, BASE_DEC, VALS(its_InformationQuality_vals), 0,
        NULL, HFILL }},
    { &hf_its_ProtectedCommunicationZonesRSU_item,
      { "ProtectedCommunicationZone", "its.ProtectedCommunicationZone_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_cenDsrcTollingZoneID,
      { "cenDsrcTollingZoneID", "its.cenDsrcTollingZoneID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_its_DigitalMap_item,
      { "ReferencePosition", "its.ReferencePosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_its_AccelerationControl_brakePedalEngaged,
      { "brakePedalEngaged", "its.brakePedalEngaged",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_its_AccelerationControl_gasPedalEngaged,
      { "gasPedalEngaged", "its.gasPedalEngaged",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_its_AccelerationControl_emergencyBrakeEngaged,
      { "emergencyBrakeEngaged", "its.emergencyBrakeEngaged",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_its_AccelerationControl_collisionWarningEngaged,
      { "collisionWarningEngaged", "its.collisionWarningEngaged",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_its_AccelerationControl_accEngaged,
      { "accEngaged", "its.accEngaged",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_its_AccelerationControl_cruiseControlEngaged,
      { "cruiseControlEngaged", "its.cruiseControlEngaged",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_its_AccelerationControl_speedLimiterEngaged,
      { "speedLimiterEngaged", "its.speedLimiterEngaged",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_its_ExteriorLights_lowBeamHeadlightsOn,
      { "lowBeamHeadlightsOn", "its.lowBeamHeadlightsOn",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_its_ExteriorLights_highBeamHeadlightsOn,
      { "highBeamHeadlightsOn", "its.highBeamHeadlightsOn",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_its_ExteriorLights_leftTurnSignalOn,
      { "leftTurnSignalOn", "its.leftTurnSignalOn",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_its_ExteriorLights_rightTurnSignalOn,
      { "rightTurnSignalOn", "its.rightTurnSignalOn",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_its_ExteriorLights_daytimeRunningLightsOn,
      { "daytimeRunningLightsOn", "its.daytimeRunningLightsOn",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_its_ExteriorLights_reverseLightOn,
      { "reverseLightOn", "its.reverseLightOn",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_its_ExteriorLights_fogLightOn,
      { "fogLightOn", "its.fogLightOn",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_its_ExteriorLights_parkingLightsOn,
      { "parkingLightsOn", "its.parkingLightsOn",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_its_SpecialTransportType_heavyLoad,
      { "heavyLoad", "its.heavyLoad",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_its_SpecialTransportType_excessWidth,
      { "excessWidth", "its.excessWidth",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_its_SpecialTransportType_excessLength,
      { "excessLength", "its.excessLength",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_its_SpecialTransportType_excessHeight,
      { "excessHeight", "its.excessHeight",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_its_LightBarSirenInUse_lightBarActivated,
      { "lightBarActivated", "its.lightBarActivated",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_its_LightBarSirenInUse_sirenActivated,
      { "sirenActivated", "its.sirenActivated",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row1LeftOccupied,
      { "row1LeftOccupied", "its.row1LeftOccupied",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row1RightOccupied,
      { "row1RightOccupied", "its.row1RightOccupied",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row1MidOccupied,
      { "row1MidOccupied", "its.row1MidOccupied",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row1NotDetectable,
      { "row1NotDetectable", "its.row1NotDetectable",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row1NotPresent,
      { "row1NotPresent", "its.row1NotPresent",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row2LeftOccupied,
      { "row2LeftOccupied", "its.row2LeftOccupied",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row2RightOccupied,
      { "row2RightOccupied", "its.row2RightOccupied",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row2MidOccupied,
      { "row2MidOccupied", "its.row2MidOccupied",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row2NotDetectable,
      { "row2NotDetectable", "its.row2NotDetectable",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row2NotPresent,
      { "row2NotPresent", "its.row2NotPresent",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row3LeftOccupied,
      { "row3LeftOccupied", "its.row3LeftOccupied",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row3RightOccupied,
      { "row3RightOccupied", "its.row3RightOccupied",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row3MidOccupied,
      { "row3MidOccupied", "its.row3MidOccupied",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row3NotDetectable,
      { "row3NotDetectable", "its.row3NotDetectable",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row3NotPresent,
      { "row3NotPresent", "its.row3NotPresent",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row4LeftOccupied,
      { "row4LeftOccupied", "its.row4LeftOccupied",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row4RightOccupied,
      { "row4RightOccupied", "its.row4RightOccupied",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row4MidOccupied,
      { "row4MidOccupied", "its.row4MidOccupied",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row4NotDetectable,
      { "row4NotDetectable", "its.row4NotDetectable",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_its_PositionOfOccupants_row4NotPresent,
      { "row4NotPresent", "its.row4NotPresent",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_its_EnergyStorageType_hydrogenStorage,
      { "hydrogenStorage", "its.hydrogenStorage",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_its_EnergyStorageType_electricEnergyStorage,
      { "electricEnergyStorage", "its.electricEnergyStorage",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_its_EnergyStorageType_liquidPropaneGas,
      { "liquidPropaneGas", "its.liquidPropaneGas",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_its_EnergyStorageType_compressedNaturalGas,
      { "compressedNaturalGas", "its.compressedNaturalGas",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_its_EnergyStorageType_diesel,
      { "diesel", "its.diesel",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_its_EnergyStorageType_gasoline,
      { "gasoline", "its.gasoline",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_its_EnergyStorageType_ammonia,
      { "ammonia", "its.ammonia",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_its_EmergencyPriority_requestForRightOfWay,
      { "requestForRightOfWay", "its.requestForRightOfWay",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_its_EmergencyPriority_requestForFreeCrossingAtATrafficLight,
      { "requestForFreeCrossingAtATrafficLight", "its.requestForFreeCrossingAtATrafficLight",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},

/* --- Module ElectronicRegistrationIdentificationVehicleDataModule --- --- --- */

    { &hf_erivdm_euVehicleCategoryL,
      { "euVehicleCategoryL", "erivdm.euVehicleCategoryL",
        FT_UINT32, BASE_DEC, VALS(erivdm_EuVehicleCategoryL_vals), 0,
        NULL, HFILL }},
    { &hf_erivdm_euVehicleCategoryM,
      { "euVehicleCategoryM", "erivdm.euVehicleCategoryM",
        FT_UINT32, BASE_DEC, VALS(erivdm_EuVehicleCategoryM_vals), 0,
        NULL, HFILL }},
    { &hf_erivdm_euVehicleCategoryN,
      { "euVehicleCategoryN", "erivdm.euVehicleCategoryN",
        FT_UINT32, BASE_DEC, VALS(erivdm_EuVehicleCategoryN_vals), 0,
        NULL, HFILL }},
    { &hf_erivdm_euVehicleCategoryO,
      { "euVehicleCategoryO", "erivdm.euVehicleCategoryO",
        FT_UINT32, BASE_DEC, VALS(erivdm_EuVehicleCategoryO_vals), 0,
        NULL, HFILL }},
    { &hf_erivdm_euVehilcleCategoryT,
      { "euVehilcleCategoryT", "erivdm.euVehilcleCategoryT_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_erivdm_euVehilcleCategoryG,
      { "euVehilcleCategoryG", "erivdm.euVehilcleCategoryG_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},

/* --- Module CITSapplMgmtIDs --- --- ---                                     */

    { &hf_csmid_vlnContent,
      { "content", "csmid.content",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_127", HFILL }},
    { &hf_csmid_vlnExtension,
      { "extension", "csmid.extension",
        FT_UINT32, BASE_DEC, VALS(csmid_Ext1_vals), 0,
        "Ext1", HFILL }},
    { &hf_csmid_e1Content,
      { "content", "csmid.content",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_128_16511", HFILL }},
    { &hf_csmid_e2Extension,
      { "extension", "csmid.extension",
        FT_UINT32, BASE_DEC, VALS(csmid_Ext2_vals), 0,
        "Ext2", HFILL }},
    { &hf_csmid_e2Content,
      { "content", "csmid.content",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_16512_2113663", HFILL }},
    { &hf_csmid_e1Extension,
      { "extension", "csmid.extension",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Ext3", HFILL }},

/* --- Module EfcDsrcApplication --- --- ---                                  */

    { &hf_dsrc_app_maxLadenweightOnAxle1,
      { "maxLadenweightOnAxle1", "dsrc_app.maxLadenweightOnAxle1",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_maxLadenweightOnAxle2,
      { "maxLadenweightOnAxle2", "dsrc_app.maxLadenweightOnAxle2",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_maxLadenweightOnAxle3,
      { "maxLadenweightOnAxle3", "dsrc_app.maxLadenweightOnAxle3",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_maxLadenweightOnAxle4,
      { "maxLadenweightOnAxle4", "dsrc_app.maxLadenweightOnAxle4",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_maxLadenweightOnAxle5,
      { "maxLadenweightOnAxle5", "dsrc_app.maxLadenweightOnAxle5",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_particulate,
      { "particulate", "dsrc_app.particulate_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_app_unitType,
      { "unitType", "dsrc_app.unitType",
        FT_UINT32, BASE_DEC, VALS(dsrc_app_UnitType_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_app_value,
      { "value", "dsrc_app.value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_32767", HFILL }},
    { &hf_dsrc_app_absorptionCoeff,
      { "absorptionCoeff", "dsrc_app.absorptionCoeff",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_euroValue,
      { "euroValue", "dsrc_app.euroValue",
        FT_UINT32, BASE_DEC, VALS(dsrc_app_EuroValue_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_app_copValue,
      { "copValue", "dsrc_app.copValue",
        FT_UINT32, BASE_DEC, VALS(dsrc_app_CopValue_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_app_emissionCO,
      { "emissionCO", "dsrc_app.emissionCO",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_32767", HFILL }},
    { &hf_dsrc_app_emissionHC,
      { "emissionHC", "dsrc_app.emissionHC",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_emissionNOX,
      { "emissionNOX", "dsrc_app.emissionNOX",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_emissionHCNOX,
      { "emissionHCNOX", "dsrc_app.emissionHCNOX",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_numberOfSeats,
      { "numberOfSeats", "dsrc_app.numberOfSeats",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int1", HFILL }},
    { &hf_dsrc_app_numberOfStandingPlaces,
      { "numberOfStandingPlaces", "dsrc_app.numberOfStandingPlaces",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int1", HFILL }},
    { &hf_dsrc_app_countryCode,
      { "countryCode", "dsrc_app.countryCode",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_app_providerIdentifier,
      { "providerIdentifier", "dsrc_app.providerIdentifier",
        FT_UINT32, BASE_DEC, NULL, 0,
        "IssuerIdentifier", HFILL }},
    { &hf_dsrc_app_soundstationary,
      { "soundstationary", "dsrc_app.soundstationary",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int1", HFILL }},
    { &hf_dsrc_app_sounddriveby,
      { "sounddriveby", "dsrc_app.sounddriveby",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int1", HFILL }},
    { &hf_dsrc_app_vehicleLengthOverall,
      { "vehicleLengthOverall", "dsrc_app.vehicleLengthOverall",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int1", HFILL }},
    { &hf_dsrc_app_vehicleHeigthOverall,
      { "vehicleHeigthOverall", "dsrc_app.vehicleHeigthOverall",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int1", HFILL }},
    { &hf_dsrc_app_vehicleWidthOverall,
      { "vehicleWidthOverall", "dsrc_app.vehicleWidthOverall",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int1", HFILL }},
    { &hf_dsrc_app_vehicleMaxLadenWeight,
      { "vehicleMaxLadenWeight", "dsrc_app.vehicleMaxLadenWeight",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_vehicleTrainMaximumWeight,
      { "vehicleTrainMaximumWeight", "dsrc_app.vehicleTrainMaximumWeight",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},
    { &hf_dsrc_app_vehicleWeightUnladen,
      { "vehicleWeightUnladen", "dsrc_app.vehicleWeightUnladen",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Int2", HFILL }},

/* --- Module IVI --- --- ---                                                 */

    { &hf_ivi_ivi_IviStructure_PDU,
      { "IviStructure", "ivi.IviStructure_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_mandatory,
      { "mandatory", "ivi.mandatory_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "IVIManagementContainer", HFILL }},
    { &hf_ivi_optional,
      { "optional", "ivi.optional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_8__OF_IviContainer", HFILL }},
    { &hf_ivi_optional_item,
      { "IviContainer", "ivi.IviContainer",
        FT_UINT32, BASE_DEC, VALS(ivi_IviContainer_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_glc,
      { "glc", "ivi.glc_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "GeographicLocationContainer", HFILL }},
    { &hf_ivi_giv,
      { "giv", "ivi.giv",
        FT_UINT32, BASE_DEC, NULL, 0,
        "GeneralIviContainer", HFILL }},
    { &hf_ivi_rcc,
      { "rcc", "ivi.rcc",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RoadConfigurationContainer", HFILL }},
    { &hf_ivi_tc,
      { "tc", "ivi.tc",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TextContainer", HFILL }},
    { &hf_ivi_lac,
      { "lac", "ivi.lac_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "LayoutContainer", HFILL }},
    { &hf_ivi_serviceProviderId,
      { "serviceProviderId", "ivi.serviceProviderId_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Provider", HFILL }},
    { &hf_ivi_iviIdentificationNumber,
      { "iviIdentificationNumber", "ivi.iviIdentificationNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_timeStamp,
      { "timeStamp", "ivi.timeStamp",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_ivi_validFrom,
      { "validFrom", "ivi.validFrom",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_ivi_validTo,
      { "validTo", "ivi.validTo",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_ivi_connectedIviStructures,
      { "connectedIviStructures", "ivi.connectedIviStructures",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_8_OF_IviIdentificationNumber", HFILL }},
    { &hf_ivi_connectedIviStructures_item,
      { "IviIdentificationNumber", "ivi.IviIdentificationNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_iviStatus,
      { "iviStatus", "ivi.iviStatus",
        FT_UINT32, BASE_DEC, VALS(ivi_IviStatus_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_referencePosition,
      { "referencePosition", "ivi.referencePosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_referencePositionTime,
      { "referencePositionTime", "ivi.referencePositionTime",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_ivi_referencePositionHeading,
      { "referencePositionHeading", "ivi.referencePositionHeading_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Heading", HFILL }},
    { &hf_ivi_referencePositionSpeed,
      { "referencePositionSpeed", "ivi.referencePositionSpeed_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Speed", HFILL }},
    { &hf_ivi_parts,
      { "parts", "ivi.parts",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_16__OF_GlcPart", HFILL }},
    { &hf_ivi_parts_item,
      { "GlcPart", "ivi.GlcPart_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_zoneId,
      { "zoneId", "ivi.zoneId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Zid", HFILL }},
    { &hf_ivi_laneNumber,
      { "laneNumber", "ivi.laneNumber",
        FT_INT32, BASE_DEC, VALS(its_LanePosition_vals), 0,
        "LanePosition", HFILL }},
    { &hf_ivi_zoneExtension,
      { "zoneExtension", "ivi.zoneExtension",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_255", HFILL }},
    { &hf_ivi_zoneHeading,
      { "zoneHeading", "ivi.zoneHeading",
        FT_UINT32, BASE_DEC, VALS(its_HeadingValue_vals), 0,
        "HeadingValue", HFILL }},
    { &hf_ivi_zone,
      { "zone", "ivi.zone",
        FT_UINT32, BASE_DEC, VALS(ivi_Zone_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_GeneralIviContainer_item,
      { "GicPart", "ivi.GicPart_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_gpDetectionZoneIds,
      { "detectionZoneIds", "ivi.detectionZoneIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_GicPartDetectionZoneIds", HFILL }},
    { &hf_ivi_gpDetectionZoneIds_item,
      { "Zid", "ivi.Zid",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_its_Rrid,
      { "its-Rrid", "ivi.its_Rrid",
        FT_UINT32, BASE_DEC, VALS(csmid_VarLengthNumber_vals), 0,
        "VarLengthNumber", HFILL }},
    { &hf_ivi_gpRelevanceZoneIds,
      { "relevanceZoneIds", "ivi.relevanceZoneIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_GicPartRelevanceZoneIds", HFILL }},
    { &hf_ivi_gpRelevanceZoneIds_item,
      { "Zid", "ivi.Zid",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_direction,
      { "direction", "ivi.direction",
        FT_UINT32, BASE_DEC, VALS(ivi_Direction_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_gpDriverAwarenessZoneIds,
      { "driverAwarenessZoneIds", "ivi.driverAwarenessZoneIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_GicPartDriverAwarenessZoneIds", HFILL }},
    { &hf_ivi_gpDriverAwarenessZoneIds_item,
      { "Zid", "ivi.Zid",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_minimumAwarenessTime,
      { "minimumAwarenessTime", "ivi.minimumAwarenessTime",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_255", HFILL }},
    { &hf_ivi_applicableLanes,
      { "applicableLanes", "ivi.applicableLanes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_8__OF_LanePosition", HFILL }},
    { &hf_ivi_applicableLanes_item,
      { "LanePosition", "ivi.LanePosition",
        FT_INT32, BASE_DEC, VALS(its_LanePosition_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_iviType,
      { "iviType", "ivi.iviType",
        FT_UINT32, BASE_DEC, VALS(ivi_IviType_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_iviPurpose,
      { "iviPurpose", "ivi.iviPurpose",
        FT_UINT32, BASE_DEC, VALS(ivi_IviPurpose_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_laneStatus,
      { "laneStatus", "ivi.laneStatus",
        FT_UINT32, BASE_DEC, VALS(ivi_LaneStatus_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_vehicleCharacteristics,
      { "vehicleCharacteristics", "ivi.vehicleCharacteristics",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_8__OF_CompleteVehicleCharacteristics", HFILL }},
    { &hf_ivi_vehicleCharacteristics_item,
      { "CompleteVehicleCharacteristics", "ivi.CompleteVehicleCharacteristics_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_driverCharacteristics,
      { "driverCharacteristics", "ivi.driverCharacteristics",
        FT_UINT32, BASE_DEC, VALS(ivi_DriverCharacteristics_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_layoutId,
      { "layoutId", "ivi.layoutId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_4_", HFILL }},
    { &hf_ivi_preStoredlayoutId,
      { "preStoredlayoutId", "ivi.preStoredlayoutId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_64_", HFILL }},
    { &hf_ivi_roadSignCodes,
      { "roadSignCodes", "ivi.roadSignCodes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_4__OF_RSCode", HFILL }},
    { &hf_ivi_roadSignCodes_item,
      { "RSCode", "ivi.RSCode_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_extraText,
      { "extraText", "ivi.extraText",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_GicPartExtraText", HFILL }},
    { &hf_ivi_extraText_item,
      { "Text", "ivi.Text_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_RoadConfigurationContainer_item,
      { "RccPart", "ivi.RccPart_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_zoneIds,
      { "zoneIds", "ivi.zoneIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_8__OF_Zid", HFILL }},
    { &hf_ivi_zoneIds_item,
      { "Zid", "ivi.Zid",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_roadType,
      { "roadType", "ivi.roadType",
        FT_UINT32, BASE_DEC, VALS(its_RoadType_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_laneConfiguration,
      { "laneConfiguration", "ivi.laneConfiguration",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_16__OF_LaneInformation", HFILL }},
    { &hf_ivi_laneConfiguration_item,
      { "LaneInformation", "ivi.LaneInformation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_TextContainer_item,
      { "TcPart", "ivi.TcPart_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_tpDetectionZoneIds,
      { "detectionZoneIds", "ivi.detectionZoneIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_TcPartDetectionZoneIds", HFILL }},
    { &hf_ivi_tpDetectionZoneIds_item,
      { "Zid", "ivi.Zid",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_tpRelevanceZoneIds,
      { "relevanceZoneIds", "ivi.relevanceZoneIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_TcPartRelevanceZoneIds", HFILL }},
    { &hf_ivi_tpRelevanceZoneIds_item,
      { "Zid", "ivi.Zid",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_tpDriverAwarenessZoneIds,
      { "driverAwarenessZoneIds", "ivi.driverAwarenessZoneIds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_TcPartDriverAwarenessZoneIds", HFILL }},
    { &hf_ivi_tpDriverAwarenessZoneIds_item,
      { "Zid", "ivi.Zid",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_text,
      { "text", "ivi.text",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_TcPartText", HFILL }},
    { &hf_ivi_text_item,
      { "Text", "ivi.Text_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_data,
      { "data", "ivi.data",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_ivi_height,
      { "height", "ivi.height",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_10_73", HFILL }},
    { &hf_ivi_width,
      { "width", "ivi.width",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_10_265", HFILL }},
    { &hf_ivi_layoutComponents,
      { "layoutComponents", "ivi.layoutComponents",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_4__OF_LayoutComponent", HFILL }},
    { &hf_ivi_layoutComponents_item,
      { "LayoutComponent", "ivi.LayoutComponent_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_latitude,
      { "latitude", "ivi.latitude",
        FT_INT32, BASE_DEC, VALS(its_Latitude_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_longitude,
      { "longitude", "ivi.longitude",
        FT_INT32, BASE_DEC, VALS(its_Longitude_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_altitude,
      { "altitude", "ivi.altitude_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_owner,
      { "owner", "ivi.owner_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Provider", HFILL }},
    { &hf_ivi_version,
      { "version", "ivi.version",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_255", HFILL }},
    { &hf_ivi_acPictogramCode,
      { "pictogramCode", "ivi.pictogramCode",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_ivi_acValue,
      { "value", "ivi.value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_ivi_unit,
      { "unit", "ivi.unit",
        FT_UINT32, BASE_DEC, VALS(ivi_RSCUnit_vals), 0,
        "RSCUnit", HFILL }},
    { &hf_ivi_attributes,
      { "attributes", "ivi.attributes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ISO14823Attributes", HFILL }},
    { &hf_ivi_tractor,
      { "tractor", "ivi.tractor_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TractorCharacteristics", HFILL }},
    { &hf_ivi_trailer,
      { "trailer", "ivi.trailer",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_3_OF_TrailerCharacteristics", HFILL }},
    { &hf_ivi_trailer_item,
      { "TrailerCharacteristics", "ivi.TrailerCharacteristics_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_train,
      { "train", "ivi.train_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TrainCharacteristics", HFILL }},
    { &hf_ivi_laneWidth,
      { "laneWidth", "ivi.laneWidth",
        FT_UINT32, BASE_DEC, NULL, 0,
        "IVILaneWidth", HFILL }},
    { &hf_ivi_offsetDistance,
      { "offsetDistance", "ivi.offsetDistance",
        FT_INT32, BASE_DEC, NULL, 0,
        "INTEGER_M32768_32767", HFILL }},
    { &hf_ivi_offsetPosition,
      { "offsetPosition", "ivi.offsetPosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "DeltaReferencePosition", HFILL }},
    { &hf_ivi_deltaLatitude,
      { "deltaLatitude", "ivi.deltaLatitude",
        FT_INT32, BASE_DEC, VALS(its_DeltaLatitude_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_deltaLongitude,
      { "deltaLongitude", "ivi.deltaLongitude",
        FT_INT32, BASE_DEC, VALS(its_DeltaLongitude_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_dValue,
      { "value", "ivi.value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_16384", HFILL }},
    { &hf_ivi_dodValue,
      { "value", "ivi.value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_16384", HFILL }},
    { &hf_ivi_ISO14823Attributes_item,
      { "ISO14823Attributes item", "ivi.ISO14823Attributes_item",
        FT_UINT32, BASE_DEC, VALS(ivi_ISO14823Attributes_item_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_dtm,
      { "dtm", "ivi.dtm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_edt,
      { "edt", "ivi.edt_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_dfl,
      { "dfl", "ivi.dfl",
        FT_UINT32, BASE_DEC, VALS(ivi_DFL_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_ved,
      { "ved", "ivi.ved_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_spe,
      { "spe", "ivi.spe_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_roi,
      { "roi", "ivi.roi",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_dbv,
      { "dbv", "ivi.dbv_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_ddd,
      { "ddd", "ivi.ddd_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_icPictogramCode,
      { "pictogramCode", "ivi.pictogramCode_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "T_icPictogramCode", HFILL }},
    { &hf_ivi_countryCode,
      { "countryCode", "ivi.countryCode",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING_SIZE_2", HFILL }},
    { &hf_ivi_serviceCategoryCode,
      { "serviceCategoryCode", "ivi.serviceCategoryCode",
        FT_UINT32, BASE_DEC, VALS(ivi_T_serviceCategoryCode_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_trafficSignPictogram,
      { "trafficSignPictogram", "ivi.trafficSignPictogram",
        FT_UINT32, BASE_DEC, VALS(ivi_T_trafficSignPictogram_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_publicFacilitiesPictogram,
      { "publicFacilitiesPictogram", "ivi.publicFacilitiesPictogram",
        FT_UINT32, BASE_DEC, VALS(ivi_T_publicFacilitiesPictogram_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_ambientOrRoadConditionPictogram,
      { "ambientOrRoadConditionPictogram", "ivi.ambientOrRoadConditionPictogram",
        FT_UINT32, BASE_DEC, VALS(ivi_T_ambientOrRoadConditionPictogram_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_pictogramCategoryCode,
      { "pictogramCategoryCode", "ivi.pictogramCategoryCode_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_nature,
      { "nature", "ivi.nature",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_9", HFILL }},
    { &hf_ivi_serialNumber,
      { "serialNumber", "ivi.serialNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_99", HFILL }},
    { &hf_ivi_liValidity,
      { "validity", "ivi.validity_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "DTM", HFILL }},
    { &hf_ivi_laneType,
      { "laneType", "ivi.laneType",
        FT_UINT32, BASE_DEC, VALS(ivi_LaneType_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_laneTypeQualifier,
      { "laneTypeQualifier", "ivi.laneTypeQualifier_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CompleteVehicleCharacteristics", HFILL }},
    { &hf_ivi_lcLayoutComponentId,
      { "layoutComponentId", "ivi.layoutComponentId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_8_", HFILL }},
    { &hf_ivi_x,
      { "x", "ivi.x",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_10_265", HFILL }},
    { &hf_ivi_y,
      { "y", "ivi.y",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_10_73", HFILL }},
    { &hf_ivi_textScripting,
      { "textScripting", "ivi.textScripting",
        FT_UINT32, BASE_DEC, VALS(ivi_T_textScripting_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_goodsType,
      { "goodsType", "ivi.goodsType",
        FT_UINT32, BASE_DEC, VALS(ivi_GoodsType_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_dangerousGoodsType,
      { "dangerousGoodsType", "ivi.dangerousGoodsType",
        FT_UINT32, BASE_DEC, VALS(its_DangerousGoodsBasic_vals), 0,
        "DangerousGoodsBasic", HFILL }},
    { &hf_ivi_specialTransportType,
      { "specialTransportType", "ivi.specialTransportType",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_deltaPositions,
      { "deltaPositions", "ivi.deltaPositions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_32__OF_DeltaPosition", HFILL }},
    { &hf_ivi_deltaPositions_item,
      { "DeltaPosition", "ivi.DeltaPosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_deltaPositionsWithAltitude,
      { "deltaPositionsWithAltitude", "ivi.deltaPositionsWithAltitude",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_32__OF_DeltaReferencePosition", HFILL }},
    { &hf_ivi_deltaPositionsWithAltitude_item,
      { "DeltaReferencePosition", "ivi.DeltaReferencePosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_absolutePositions,
      { "absolutePositions", "ivi.absolutePositions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_8__OF_AbsolutePosition", HFILL }},
    { &hf_ivi_absolutePositions_item,
      { "AbsolutePosition", "ivi.AbsolutePosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_absolutePositionsWithAltitude,
      { "absolutePositionsWithAltitude", "ivi.absolutePositionsWithAltitude",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_8__OF_AbsolutePositionWAltitude", HFILL }},
    { &hf_ivi_absolutePositionsWithAltitude_item,
      { "AbsolutePositionWAltitude", "ivi.AbsolutePositionWAltitude_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_rscLayoutComponentId,
      { "layoutComponentId", "ivi.layoutComponentId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_4_", HFILL }},
    { &hf_ivi_code,
      { "code", "ivi.code",
        FT_UINT32, BASE_DEC, VALS(ivi_T_code_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_viennaConvention,
      { "viennaConvention", "ivi.viennaConvention_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "VcCode", HFILL }},
    { &hf_ivi_iso14823,
      { "iso14823", "ivi.iso14823_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ISO14823Code", HFILL }},
    { &hf_ivi_itisCodes,
      { "itisCodes", "ivi.itisCodes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_ivi_anyCatalogue,
      { "anyCatalogue", "ivi.anyCatalogue_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_line,
      { "line", "ivi.line",
        FT_UINT32, BASE_DEC, VALS(ivi_PolygonalLine_vals), 0,
        "PolygonalLine", HFILL }},
    { &hf_ivi_tLayoutComponentId,
      { "layoutComponentId", "ivi.layoutComponentId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_4_", HFILL }},
    { &hf_ivi_language,
      { "language", "ivi.language",
        FT_BYTES, BASE_NONE, NULL, 0,
        "BIT_STRING_SIZE_10", HFILL }},
    { &hf_ivi_textContent,
      { "textContent", "ivi.textContent",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_ivi_toEqualTo,
      { "equalTo", "ivi.equalTo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_TractorCharactEqualTo", HFILL }},
    { &hf_ivi_toEqualTo_item,
      { "VehicleCharacteristicsFixValues", "ivi.VehicleCharacteristicsFixValues",
        FT_UINT32, BASE_DEC, VALS(ivi_VehicleCharacteristicsFixValues_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_toNotEqualTo,
      { "notEqualTo", "ivi.notEqualTo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_TractorCharactNotEqualTo", HFILL }},
    { &hf_ivi_toNotEqualTo_item,
      { "VehicleCharacteristicsFixValues", "ivi.VehicleCharacteristicsFixValues",
        FT_UINT32, BASE_DEC, VALS(ivi_VehicleCharacteristicsFixValues_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_ranges,
      { "ranges", "ivi.ranges",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_4__OF_VehicleCharacteristicsRanges", HFILL }},
    { &hf_ivi_ranges_item,
      { "VehicleCharacteristicsRanges", "ivi.VehicleCharacteristicsRanges_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_teEqualTo,
      { "equalTo", "ivi.equalTo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_TrailerCharactEqualTo", HFILL }},
    { &hf_ivi_teEqualTo_item,
      { "VehicleCharacteristicsFixValues", "ivi.VehicleCharacteristicsFixValues",
        FT_UINT32, BASE_DEC, VALS(ivi_VehicleCharacteristicsFixValues_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_teNotEqualTo,
      { "notEqualTo", "ivi.notEqualTo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_TrailerCharactNotEqualTo", HFILL }},
    { &hf_ivi_teNotEqualTo_item,
      { "VehicleCharacteristicsFixValues", "ivi.VehicleCharacteristicsFixValues",
        FT_UINT32, BASE_DEC, VALS(ivi_VehicleCharacteristicsFixValues_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_roadSignClass,
      { "roadSignClass", "ivi.roadSignClass",
        FT_UINT32, BASE_DEC, VALS(ivi_VcClass_vals), 0,
        "VcClass", HFILL }},
    { &hf_ivi_roadSignCode,
      { "roadSignCode", "ivi.roadSignCode",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_64", HFILL }},
    { &hf_ivi_vcOption,
      { "vcOption", "ivi.vcOption",
        FT_UINT32, BASE_DEC, VALS(ivi_VcOption_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_vcValidity,
      { "validity", "ivi.validity",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_8__OF_DTM", HFILL }},
    { &hf_ivi_vcValidity_item,
      { "DTM", "ivi.DTM_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_vcValue,
      { "value", "ivi.value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_65535", HFILL }},
    { &hf_ivi_simpleVehicleType,
      { "simpleVehicleType", "ivi.simpleVehicleType",
        FT_UINT32, BASE_DEC, VALS(its_StationType_vals), 0,
        "StationType", HFILL }},
    { &hf_ivi_euVehicleCategoryCode,
      { "euVehicleCategoryCode", "ivi.euVehicleCategoryCode",
        FT_UINT32, BASE_DEC, VALS(erivdm_EuVehicleCategoryCode_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_iso3833VehicleType,
      { "iso3833VehicleType", "ivi.iso3833VehicleType",
        FT_UINT32, BASE_DEC, VALS(erivdm_Iso3833VehicleType_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_euroAndCo2value,
      { "euroAndCo2value", "ivi.euroAndCo2value_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "EnvironmentalCharacteristics", HFILL }},
    { &hf_ivi_engineCharacteristics,
      { "engineCharacteristics", "ivi.engineCharacteristics",
        FT_UINT32, BASE_DEC, VALS(dsrc_app_EngineCharacteristics_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_loadType,
      { "loadType", "ivi.loadType_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_usage,
      { "usage", "ivi.usage",
        FT_UINT32, BASE_DEC, VALS(its_VehicleRole_vals), 0,
        "VehicleRole", HFILL }},
    { &hf_ivi_comparisonOperator,
      { "comparisonOperator", "ivi.comparisonOperator",
        FT_UINT32, BASE_DEC, VALS(ivi_ComparisonOperator_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_limits,
      { "limits", "ivi.limits",
        FT_UINT32, BASE_DEC, VALS(ivi_T_limits_vals), 0,
        NULL, HFILL }},
    { &hf_ivi_numberOfAxles,
      { "numberOfAxles", "ivi.numberOfAxles",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_7", HFILL }},
    { &hf_ivi_vehicleDimensions,
      { "vehicleDimensions", "ivi.vehicleDimensions_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_vehicleWeightLimits,
      { "vehicleWeightLimits", "ivi.vehicleWeightLimits_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_axleWeightLimits,
      { "axleWeightLimits", "ivi.axleWeightLimits_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_passengerCapacity,
      { "passengerCapacity", "ivi.passengerCapacity_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_exhaustEmissionValues,
      { "exhaustEmissionValues", "ivi.exhaustEmissionValues_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_dieselEmissionValues,
      { "dieselEmissionValues", "ivi.dieselEmissionValues_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_soundLevel,
      { "soundLevel", "ivi.soundLevel_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_wValue,
      { "value", "ivi.value",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_16384", HFILL }},
    { &hf_ivi_segment,
      { "segment", "ivi.segment_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_area,
      { "area", "ivi.area",
        FT_UINT32, BASE_DEC, VALS(ivi_PolygonalLine_vals), 0,
        "PolygonalLine", HFILL }},
    { &hf_ivi_computedSegment,
      { "computedSegment", "ivi.computedSegment_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_year,
      { "year", "ivi.year_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_syr,
      { "syr", "ivi.syr",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_2000_2127_", HFILL }},
    { &hf_ivi_eyr,
      { "eyr", "ivi.eyr",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_2000_2127_", HFILL }},
    { &hf_ivi_month_day,
      { "month-day", "ivi.month_day_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_smd,
      { "smd", "ivi.smd_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "MonthDay", HFILL }},
    { &hf_ivi_emd,
      { "emd", "ivi.emd_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "MonthDay", HFILL }},
    { &hf_ivi_pmd,
      { "pmd", "ivi.pmd",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_hourMinutes,
      { "hourMinutes", "ivi.hourMinutes_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_shm,
      { "shm", "ivi.shm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "HoursMinutes", HFILL }},
    { &hf_ivi_ehm,
      { "ehm", "ivi.ehm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "HoursMinutes", HFILL }},
    { &hf_ivi_dayOfWeek,
      { "dayOfWeek", "ivi.dayOfWeek",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_period,
      { "period", "ivi.period_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "HoursMinutes", HFILL }},
    { &hf_ivi_month,
      { "month", "ivi.month",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_12", HFILL }},
    { &hf_ivi_day,
      { "day", "ivi.day",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_31", HFILL }},
    { &hf_ivi_hours,
      { "hours", "ivi.hours",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_23", HFILL }},
    { &hf_ivi_mins,
      { "mins", "ivi.mins",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_59", HFILL }},
    { &hf_ivi_hei,
      { "hei", "ivi.hei_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Distance", HFILL }},
    { &hf_ivi_wid,
      { "wid", "ivi.wid_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Distance", HFILL }},
    { &hf_ivi_vln,
      { "vln", "ivi.vln_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Distance", HFILL }},
    { &hf_ivi_wei,
      { "wei", "ivi.wei_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Weight", HFILL }},
    { &hf_ivi_spm,
      { "spm", "ivi.spm",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_250", HFILL }},
    { &hf_ivi_mns,
      { "mns", "ivi.mns",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_250", HFILL }},
    { &hf_ivi_dcj,
      { "dcj", "ivi.dcj",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_128", HFILL }},
    { &hf_ivi_dcr,
      { "dcr", "ivi.dcr",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_128", HFILL }},
    { &hf_ivi_tpl,
      { "tpl", "ivi.tpl",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_128", HFILL }},
    { &hf_ivi_ioList,
      { "ioList", "ivi.ioList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_8__OF_DDD_IO", HFILL }},
    { &hf_ivi_ioList_item,
      { "DDD-IO", "ivi.DDD_IO_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_drn,
      { "drn", "ivi.drn",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_7", HFILL }},
    { &hf_ivi_dp,
      { "dp", "ivi.dp",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_4__OF_DestinationPlace", HFILL }},
    { &hf_ivi_dp_item,
      { "DestinationPlace", "ivi.DestinationPlace_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_dr,
      { "dr", "ivi.dr",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_4__OF_DestinationRoad", HFILL }},
    { &hf_ivi_dr_item,
      { "DestinationRoad", "ivi.DestinationRoad_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_ivi_rne,
      { "rne", "ivi.rne",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_999", HFILL }},
    { &hf_ivi_stnId,
      { "stnId", "ivi.stnId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_999", HFILL }},
    { &hf_ivi_stnText,
      { "stnText", "ivi.stnText",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_ivi_dcp,
      { "dcp", "ivi.dcp_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "DistanceOrDuration", HFILL }},
    { &hf_ivi_ddp,
      { "ddp", "ivi.ddp_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "DistanceOrDuration", HFILL }},
    { &hf_ivi_depType,
      { "depType", "ivi.depType",
        FT_UINT32, BASE_DEC, VALS(ivi_DDD_DEP_vals), 0,
        "DDD_DEP", HFILL }},
    { &hf_ivi_depRSCode,
      { "depRSCode", "ivi.depRSCode_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ISO14823Code", HFILL }},
    { &hf_ivi_depBlob,
      { "depBlob", "ivi.depBlob",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_ivi_plnId,
      { "plnId", "ivi.plnId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_999", HFILL }},
    { &hf_ivi_plnText,
      { "plnText", "ivi.plnText",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_ivi_derType,
      { "derType", "ivi.derType",
        FT_UINT32, BASE_DEC, VALS(ivi_DDD_DER_vals), 0,
        "DDD_DER", HFILL }},
    { &hf_ivi_ronId,
      { "ronId", "ivi.ronId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_999", HFILL }},
    { &hf_ivi_ronText,
      { "ronText", "ivi.ronText",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_ivi_PMD_national_holiday,
      { "national-holiday", "ivi.national-holiday",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_ivi_PMD_even_days,
      { "even-days", "ivi.even-days",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_ivi_PMD_odd_days,
      { "odd-days", "ivi.odd-days",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_ivi_PMD_market_day,
      { "market-day", "ivi.market-day",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_ivi_DayOfWeek_unused,
      { "unused", "ivi.unused",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_ivi_DayOfWeek_monday,
      { "monday", "ivi.monday",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_ivi_DayOfWeek_tuesday,
      { "tuesday", "ivi.tuesday",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_ivi_DayOfWeek_wednesday,
      { "wednesday", "ivi.wednesday",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_ivi_DayOfWeek_thursday,
      { "thursday", "ivi.thursday",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_ivi_DayOfWeek_friday,
      { "friday", "ivi.friday",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_ivi_DayOfWeek_saturday,
      { "saturday", "ivi.saturday",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_ivi_DayOfWeek_sunday,
      { "sunday", "ivi.sunday",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},

/* --- Module DSRC --- --- ---                                                */

    { &hf_dsrc_dsrc_SPAT_PDU,
      { "SPAT", "dsrc.SPAT_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_dsrc_SignalRequestMessage_PDU,
      { "SignalRequestMessage", "dsrc.SignalRequestMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_dsrc_SignalStatusMessage_PDU,
      { "SignalStatusMessage", "dsrc.SignalStatusMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_dsrc_MapData_PDU,
      { "MapData", "dsrc.MapData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_regionId,
      { "regionId", "dsrc.regionId",
        FT_UINT32, BASE_DEC, VALS(dsrc_RegionId_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_regExtValue,
      { "regExtValue", "dsrc.regExtValue_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_spatTimeStamp,
      { "timeStamp", "dsrc.timeStamp",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MinuteOfTheYear", HFILL }},
    { &hf_dsrc_name,
      { "name", "dsrc.name",
        FT_STRING, BASE_NONE, NULL, 0,
        "DescriptiveName", HFILL }},
    { &hf_dsrc_spatIntersections,
      { "intersections", "dsrc.intersections",
        FT_UINT32, BASE_DEC, NULL, 0,
        "IntersectionStateList", HFILL }},
    { &hf_dsrc_spatRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_SPATRegional", HFILL }},
    { &hf_dsrc_spatRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_srmTimeStamp,
      { "timeStamp", "dsrc.timeStamp",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MinuteOfTheYear", HFILL }},
    { &hf_dsrc_second,
      { "second", "dsrc.second",
        FT_UINT32, BASE_DEC, NULL, 0,
        "DSecond", HFILL }},
    { &hf_dsrc_sequenceNumber,
      { "sequenceNumber", "dsrc.sequenceNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MsgCount", HFILL }},
    { &hf_dsrc_requests,
      { "requests", "dsrc.requests",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignalRequestList", HFILL }},
    { &hf_dsrc_requestor,
      { "requestor", "dsrc.requestor_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestorDescription", HFILL }},
    { &hf_dsrc_srmRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_SRMRegional", HFILL }},
    { &hf_dsrc_srmRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_ssmTimeStamp,
      { "timeStamp", "dsrc.timeStamp",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MinuteOfTheYear", HFILL }},
    { &hf_dsrc_ssmStatus,
      { "status", "dsrc.status",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignalStatusList", HFILL }},
    { &hf_dsrc_ssmRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_SSMRegional", HFILL }},
    { &hf_dsrc_ssmRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_mdTimeStamp,
      { "timeStamp", "dsrc.timeStamp",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MinuteOfTheYear", HFILL }},
    { &hf_dsrc_msgIssueRevision,
      { "msgIssueRevision", "dsrc.msgIssueRevision",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MsgCount", HFILL }},
    { &hf_dsrc_layerType,
      { "layerType", "dsrc.layerType",
        FT_UINT32, BASE_DEC, VALS(dsrc_LayerType_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_layerID,
      { "layerID", "dsrc.layerID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_mdIntersections,
      { "intersections", "dsrc.intersections",
        FT_UINT32, BASE_DEC, NULL, 0,
        "IntersectionGeometryList", HFILL }},
    { &hf_dsrc_roadSegments,
      { "roadSegments", "dsrc.roadSegments",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RoadSegmentList", HFILL }},
    { &hf_dsrc_dataParameters,
      { "dataParameters", "dsrc.dataParameters_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_restrictionList,
      { "restrictionList", "dsrc.restrictionList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RestrictionClassList", HFILL }},
    { &hf_dsrc_mapRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_MAPRegional", HFILL }},
    { &hf_dsrc_mapRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_asType,
      { "type", "dsrc.type",
        FT_UINT32, BASE_DEC, VALS(dsrc_AdvisorySpeedType_vals), 0,
        "AdvisorySpeedType", HFILL }},
    { &hf_dsrc_asSpeed,
      { "speed", "dsrc.speed",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SpeedAdvice", HFILL }},
    { &hf_dsrc_asConfidence,
      { "confidence", "dsrc.confidence",
        FT_UINT32, BASE_DEC, VALS(its_SpeedConfidence_vals), 0,
        "SpeedConfidence", HFILL }},
    { &hf_dsrc_distance,
      { "distance", "dsrc.distance",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ZoneLength", HFILL }},
    { &hf_dsrc_class,
      { "class", "dsrc.class",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RestrictionClassID", HFILL }},
    { &hf_dsrc_asRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_AdvisorySpeedRegional", HFILL }},
    { &hf_dsrc_asRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_AdvisorySpeedList_item,
      { "AdvisorySpeed", "dsrc.AdvisorySpeed_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_referenceLaneId,
      { "referenceLaneId", "dsrc.referenceLaneId",
        FT_UINT32, BASE_DEC, NULL, 0,
        "LaneID", HFILL }},
    { &hf_dsrc_offsetXaxis,
      { "offsetXaxis", "dsrc.offsetXaxis",
        FT_UINT32, BASE_DEC, VALS(dsrc_T_offsetXaxis_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_small,
      { "small", "dsrc.small",
        FT_INT32, BASE_DEC, NULL, 0,
        "DrivenLineOffsetSm", HFILL }},
    { &hf_dsrc_large,
      { "large", "dsrc.large",
        FT_INT32, BASE_DEC, NULL, 0,
        "DrivenLineOffsetLg", HFILL }},
    { &hf_dsrc_offsetYaxis,
      { "offsetYaxis", "dsrc.offsetYaxis",
        FT_UINT32, BASE_DEC, VALS(dsrc_T_offsetYaxis_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_rotateXY,
      { "rotateXY", "dsrc.rotateXY",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Angle", HFILL }},
    { &hf_dsrc_scaleXaxis,
      { "scaleXaxis", "dsrc.scaleXaxis",
        FT_INT32, BASE_DEC, NULL, 0,
        "Scale_B12", HFILL }},
    { &hf_dsrc_scaleYaxis,
      { "scaleYaxis", "dsrc.scaleYaxis",
        FT_INT32, BASE_DEC, NULL, 0,
        "Scale_B12", HFILL }},
    { &hf_dsrc_clRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_ComputedLaneRegional", HFILL }},
    { &hf_dsrc_clRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_ConnectsToList_item,
      { "Connection", "dsrc.Connection_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_lane,
      { "lane", "dsrc.lane",
        FT_UINT32, BASE_DEC, NULL, 0,
        "LaneID", HFILL }},
    { &hf_dsrc_maneuver,
      { "maneuver", "dsrc.maneuver",
        FT_BYTES, BASE_NONE, NULL, 0,
        "AllowedManeuvers", HFILL }},
    { &hf_dsrc_connectingLane,
      { "connectingLane", "dsrc.connectingLane_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_remoteIntersection,
      { "remoteIntersection", "dsrc.remoteIntersection_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "IntersectionReferenceID", HFILL }},
    { &hf_dsrc_signalGroup,
      { "signalGroup", "dsrc.signalGroup",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignalGroupID", HFILL }},
    { &hf_dsrc_userClass,
      { "userClass", "dsrc.userClass",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RestrictionClassID", HFILL }},
    { &hf_dsrc_connectionID,
      { "connectionID", "dsrc.connectionID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "LaneConnectionID", HFILL }},
    { &hf_dsrc_queueLength,
      { "queueLength", "dsrc.queueLength",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ZoneLength", HFILL }},
    { &hf_dsrc_availableStorageLength,
      { "availableStorageLength", "dsrc.availableStorageLength",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ZoneLength", HFILL }},
    { &hf_dsrc_waitOnStop,
      { "waitOnStop", "dsrc.waitOnStop",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "WaitOnStopline", HFILL }},
    { &hf_dsrc_pedBicycleDetect,
      { "pedBicycleDetect", "dsrc.pedBicycleDetect",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "PedestrianBicycleDetect", HFILL }},
    { &hf_dsrc_cmaRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_ConnectionManeuverAssistRegional", HFILL }},
    { &hf_dsrc_cmaRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_processMethod,
      { "processMethod", "dsrc.processMethod",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_255", HFILL }},
    { &hf_dsrc_processAgency,
      { "processAgency", "dsrc.processAgency",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_255", HFILL }},
    { &hf_dsrc_lastCheckedDate,
      { "lastCheckedDate", "dsrc.lastCheckedDate",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_255", HFILL }},
    { &hf_dsrc_geoidUsed,
      { "geoidUsed", "dsrc.geoidUsed",
        FT_STRING, BASE_NONE, NULL, 0,
        "IA5String_SIZE_1_255", HFILL }},
    { &hf_dsrc_EnabledLaneList_item,
      { "LaneID", "dsrc.LaneID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_laneID,
      { "laneID", "dsrc.laneID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_ingressApproach,
      { "ingressApproach", "dsrc.ingressApproach",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ApproachID", HFILL }},
    { &hf_dsrc_egressApproach,
      { "egressApproach", "dsrc.egressApproach",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ApproachID", HFILL }},
    { &hf_dsrc_laneAttributes,
      { "laneAttributes", "dsrc.laneAttributes_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_maneuvers,
      { "maneuvers", "dsrc.maneuvers",
        FT_BYTES, BASE_NONE, NULL, 0,
        "AllowedManeuvers", HFILL }},
    { &hf_dsrc_nodeList,
      { "nodeList", "dsrc.nodeList",
        FT_UINT32, BASE_DEC, VALS(dsrc_NodeListXY_vals), 0,
        "NodeListXY", HFILL }},
    { &hf_dsrc_connectsTo,
      { "connectsTo", "dsrc.connectsTo",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ConnectsToList", HFILL }},
    { &hf_dsrc_overlays,
      { "overlays", "dsrc.overlays",
        FT_UINT32, BASE_DEC, NULL, 0,
        "OverlayLaneList", HFILL }},
    { &hf_dsrc_glRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_GenericLaneRegional", HFILL }},
    { &hf_dsrc_glRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_approach,
      { "approach", "dsrc.approach",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ApproachID", HFILL }},
    { &hf_dsrc_connection,
      { "connection", "dsrc.connection",
        FT_UINT32, BASE_DEC, NULL, 0,
        "LaneConnectionID", HFILL }},
    { &hf_dsrc_igId,
      { "id", "dsrc.id_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "IntersectionReferenceID", HFILL }},
    { &hf_dsrc_revision,
      { "revision", "dsrc.revision",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MsgCount", HFILL }},
    { &hf_dsrc_refPoint,
      { "refPoint", "dsrc.refPoint_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Position3D", HFILL }},
    { &hf_dsrc_laneWidth,
      { "laneWidth", "dsrc.laneWidth",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_speedLimits,
      { "speedLimits", "dsrc.speedLimits",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SpeedLimitList", HFILL }},
    { &hf_dsrc_laneSet,
      { "laneSet", "dsrc.laneSet",
        FT_UINT32, BASE_DEC, NULL, 0,
        "LaneList", HFILL }},
    { &hf_dsrc_preemptPriorityData,
      { "preemptPriorityData", "dsrc.preemptPriorityData",
        FT_UINT32, BASE_DEC, NULL, 0,
        "PreemptPriorityList", HFILL }},
    { &hf_dsrc_igRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_IntersectionGeometryRegional", HFILL }},
    { &hf_dsrc_igRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionGeometryList_item,
      { "IntersectionGeometry", "dsrc.IntersectionGeometry_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_region,
      { "region", "dsrc.region",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RoadRegulatorID", HFILL }},
    { &hf_dsrc_irId,
      { "id", "dsrc.id",
        FT_UINT32, BASE_DEC, NULL, 0,
        "IntersectionID", HFILL }},
    { &hf_dsrc_isId,
      { "id", "dsrc.id_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "IntersectionReferenceID", HFILL }},
    { &hf_dsrc_isStatus,
      { "status", "dsrc.status",
        FT_BYTES, BASE_NONE, NULL, 0,
        "IntersectionStatusObject", HFILL }},
    { &hf_dsrc_moy,
      { "moy", "dsrc.moy",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MinuteOfTheYear", HFILL }},
    { &hf_dsrc_isTimeStamp,
      { "timeStamp", "dsrc.timeStamp",
        FT_UINT32, BASE_DEC, NULL, 0,
        "DSecond", HFILL }},
    { &hf_dsrc_enabledLanes,
      { "enabledLanes", "dsrc.enabledLanes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "EnabledLaneList", HFILL }},
    { &hf_dsrc_states,
      { "states", "dsrc.states",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MovementList", HFILL }},
    { &hf_dsrc_maneuverAssistList,
      { "maneuverAssistList", "dsrc.maneuverAssistList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_isRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_IntersectionStateRegional", HFILL }},
    { &hf_dsrc_isRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStateList_item,
      { "IntersectionState", "dsrc.IntersectionState_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_directionalUse,
      { "directionalUse", "dsrc.directionalUse",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LaneDirection", HFILL }},
    { &hf_dsrc_sharedWith,
      { "sharedWith", "dsrc.sharedWith",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LaneSharing", HFILL }},
    { &hf_dsrc_laneType,
      { "laneType", "dsrc.laneType",
        FT_UINT32, BASE_DEC, VALS(dsrc_LaneTypeAttributes_vals), 0,
        "LaneTypeAttributes", HFILL }},
    { &hf_dsrc_laRegional,
      { "regional", "dsrc.regional_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RegionalExtension", HFILL }},
    { &hf_dsrc_pathEndPointAngle,
      { "pathEndPointAngle", "dsrc.pathEndPointAngle",
        FT_INT32, BASE_DEC, NULL, 0,
        "DeltaAngle", HFILL }},
    { &hf_dsrc_laneCrownPointCenter,
      { "laneCrownPointCenter", "dsrc.laneCrownPointCenter",
        FT_INT32, BASE_DEC, NULL, 0,
        "RoadwayCrownAngle", HFILL }},
    { &hf_dsrc_laneCrownPointLeft,
      { "laneCrownPointLeft", "dsrc.laneCrownPointLeft",
        FT_INT32, BASE_DEC, NULL, 0,
        "RoadwayCrownAngle", HFILL }},
    { &hf_dsrc_laneCrownPointRight,
      { "laneCrownPointRight", "dsrc.laneCrownPointRight",
        FT_INT32, BASE_DEC, NULL, 0,
        "RoadwayCrownAngle", HFILL }},
    { &hf_dsrc_laneAngle,
      { "laneAngle", "dsrc.laneAngle",
        FT_INT32, BASE_DEC, NULL, 0,
        "MergeDivergeNodeAngle", HFILL }},
    { &hf_dsrc_ldaRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_LaneDataAttributeRegional", HFILL }},
    { &hf_dsrc_ldaRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_LaneDataAttributeList_item,
      { "LaneDataAttribute", "dsrc.LaneDataAttribute",
        FT_UINT32, BASE_DEC, VALS(dsrc_LaneDataAttribute_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_LaneList_item,
      { "GenericLane", "dsrc.GenericLane_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_vehicle,
      { "vehicle", "dsrc.vehicle",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LaneAttributes_Vehicle", HFILL }},
    { &hf_dsrc_crosswalk,
      { "crosswalk", "dsrc.crosswalk",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LaneAttributes_Crosswalk", HFILL }},
    { &hf_dsrc_bikeLane,
      { "bikeLane", "dsrc.bikeLane",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LaneAttributes_Bike", HFILL }},
    { &hf_dsrc_sidewalk,
      { "sidewalk", "dsrc.sidewalk",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LaneAttributes_Sidewalk", HFILL }},
    { &hf_dsrc_median,
      { "median", "dsrc.median",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LaneAttributes_Barrier", HFILL }},
    { &hf_dsrc_striping,
      { "striping", "dsrc.striping",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LaneAttributes_Striping", HFILL }},
    { &hf_dsrc_trackedVehicle,
      { "trackedVehicle", "dsrc.trackedVehicle",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LaneAttributes_TrackedVehicle", HFILL }},
    { &hf_dsrc_parking,
      { "parking", "dsrc.parking",
        FT_BYTES, BASE_NONE, NULL, 0,
        "LaneAttributes_Parking", HFILL }},
    { &hf_dsrc_ManeuverAssistList_item,
      { "ConnectionManeuverAssist", "dsrc.ConnectionManeuverAssist_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_MovementEventList_item,
      { "MovementEvent", "dsrc.MovementEvent_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_eventState,
      { "eventState", "dsrc.eventState",
        FT_UINT32, BASE_DEC, VALS(dsrc_MovementPhaseState_vals), 0,
        "MovementPhaseState", HFILL }},
    { &hf_dsrc_timing,
      { "timing", "dsrc.timing_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TimeChangeDetails", HFILL }},
    { &hf_dsrc_speeds,
      { "speeds", "dsrc.speeds",
        FT_UINT32, BASE_DEC, NULL, 0,
        "AdvisorySpeedList", HFILL }},
    { &hf_dsrc_meRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_MovementEventRegional", HFILL }},
    { &hf_dsrc_meRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_MovementList_item,
      { "MovementState", "dsrc.MovementState_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_movementName,
      { "movementName", "dsrc.movementName",
        FT_STRING, BASE_NONE, NULL, 0,
        "DescriptiveName", HFILL }},
    { &hf_dsrc_state_time_speed,
      { "state-time-speed", "dsrc.state_time_speed",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MovementEventList", HFILL }},
    { &hf_dsrc_msRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_MovementStateRegional", HFILL }},
    { &hf_dsrc_msRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_localNode,
      { "localNode", "dsrc.localNode",
        FT_UINT32, BASE_DEC, NULL, 0,
        "NodeAttributeXYList", HFILL }},
    { &hf_dsrc_disabled,
      { "disabled", "dsrc.disabled",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SegmentAttributeXYList", HFILL }},
    { &hf_dsrc_enabled,
      { "enabled", "dsrc.enabled",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SegmentAttributeXYList", HFILL }},
    { &hf_dsrc_data,
      { "data", "dsrc.data",
        FT_UINT32, BASE_DEC, NULL, 0,
        "LaneDataAttributeList", HFILL }},
    { &hf_dsrc_dWidth,
      { "dWidth", "dsrc.dWidth",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B10", HFILL }},
    { &hf_dsrc_dElevation,
      { "dElevation", "dsrc.dElevation",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B10", HFILL }},
    { &hf_dsrc_nasxyRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_NodeAttributeSetXYRegional", HFILL }},
    { &hf_dsrc_nasxyRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_NodeAttributeXYList_item,
      { "NodeAttributeXY", "dsrc.NodeAttributeXY",
        FT_UINT32, BASE_DEC, VALS(dsrc_NodeAttributeXY_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_lon,
      { "lon", "dsrc.lon",
        FT_INT32, BASE_DEC, VALS(its_Longitude_vals), 0,
        "Longitude", HFILL }},
    { &hf_dsrc_lat,
      { "lat", "dsrc.lat",
        FT_INT32, BASE_DEC, VALS(its_Latitude_vals), 0,
        "Latitude", HFILL }},
    { &hf_dsrc_n20bX,
      { "x", "dsrc.x",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B10", HFILL }},
    { &hf_dsrc_n20bY,
      { "y", "dsrc.y",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B10", HFILL }},
    { &hf_dsrc_n22bX,
      { "x", "dsrc.x",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B11", HFILL }},
    { &hf_dsrc_n22bY,
      { "y", "dsrc.y",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B11", HFILL }},
    { &hf_dsrc_n24bX,
      { "x", "dsrc.x",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B12", HFILL }},
    { &hf_dsrc_n24bY,
      { "y", "dsrc.y",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B12", HFILL }},
    { &hf_dsrc_n26bX,
      { "x", "dsrc.x",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B13", HFILL }},
    { &hf_dsrc_n26bY,
      { "y", "dsrc.y",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B13", HFILL }},
    { &hf_dsrc_n28bX,
      { "x", "dsrc.x",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B14", HFILL }},
    { &hf_dsrc_n28bY,
      { "y", "dsrc.y",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B14", HFILL }},
    { &hf_dsrc_n32bX,
      { "x", "dsrc.x",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B16", HFILL }},
    { &hf_dsrc_n32bY,
      { "y", "dsrc.y",
        FT_INT32, BASE_DEC, NULL, 0,
        "Offset_B16", HFILL }},
    { &hf_dsrc_nodes,
      { "nodes", "dsrc.nodes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "NodeSetXY", HFILL }},
    { &hf_dsrc_computed,
      { "computed", "dsrc.computed_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ComputedLane", HFILL }},
    { &hf_dsrc_node_XY1,
      { "node-XY1", "dsrc.node_XY1_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Node_XY_20b", HFILL }},
    { &hf_dsrc_node_XY2,
      { "node-XY2", "dsrc.node_XY2_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Node_XY_22b", HFILL }},
    { &hf_dsrc_node_XY3,
      { "node-XY3", "dsrc.node_XY3_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Node_XY_24b", HFILL }},
    { &hf_dsrc_node_XY4,
      { "node-XY4", "dsrc.node_XY4_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Node_XY_26b", HFILL }},
    { &hf_dsrc_node_XY5,
      { "node-XY5", "dsrc.node_XY5_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Node_XY_28b", HFILL }},
    { &hf_dsrc_node_XY6,
      { "node-XY6", "dsrc.node_XY6_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Node_XY_32b", HFILL }},
    { &hf_dsrc_node_LatLon,
      { "node-LatLon", "dsrc.node_LatLon_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Node_LLmD_64b", HFILL }},
    { &hf_dsrc_nopxyRegional,
      { "regional", "dsrc.regional_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RegionalExtension", HFILL }},
    { &hf_dsrc_NodeSetXY_item,
      { "NodeXY", "dsrc.NodeXY_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_delta,
      { "delta", "dsrc.delta",
        FT_UINT32, BASE_DEC, VALS(dsrc_NodeOffsetPointXY_vals), 0,
        "NodeOffsetPointXY", HFILL }},
    { &hf_dsrc_attributes,
      { "attributes", "dsrc.attributes_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "NodeAttributeSetXY", HFILL }},
    { &hf_dsrc_OverlayLaneList_item,
      { "LaneID", "dsrc.LaneID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_long,
      { "long", "dsrc.long",
        FT_INT32, BASE_DEC, VALS(its_Longitude_vals), 0,
        "Longitude", HFILL }},
    { &hf_dsrc_elevation,
      { "elevation", "dsrc.elevation",
        FT_INT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_p3dRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_Position3DRegional", HFILL }},
    { &hf_dsrc_p3dRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_PreemptPriorityList_item,
      { "SignalControlZone", "dsrc.SignalControlZone_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_rslType,
      { "type", "dsrc.type",
        FT_UINT32, BASE_DEC, VALS(dsrc_SpeedLimitType_vals), 0,
        "SpeedLimitType", HFILL }},
    { &hf_dsrc_rslSpeed,
      { "speed", "dsrc.speed",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Velocity", HFILL }},
    { &hf_dsrc_rdId,
      { "id", "dsrc.id",
        FT_UINT32, BASE_DEC, VALS(dsrc_VehicleID_vals), 0,
        "VehicleID", HFILL }},
    { &hf_dsrc_rdType,
      { "type", "dsrc.type_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestorType", HFILL }},
    { &hf_dsrc_rdPosition,
      { "position", "dsrc.position_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestorPositionVector", HFILL }},
    { &hf_dsrc_routeName,
      { "routeName", "dsrc.routeName",
        FT_STRING, BASE_NONE, NULL, 0,
        "DescriptiveName", HFILL }},
    { &hf_dsrc_transitStatus,
      { "transitStatus", "dsrc.transitStatus",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TransitVehicleStatus", HFILL }},
    { &hf_dsrc_transitOccupancy,
      { "transitOccupancy", "dsrc.transitOccupancy",
        FT_UINT32, BASE_DEC, VALS(dsrc_TransitVehicleOccupancy_vals), 0,
        "TransitVehicleOccupancy", HFILL }},
    { &hf_dsrc_transitSchedule,
      { "transitSchedule", "dsrc.transitSchedule",
        FT_INT32, BASE_DEC, NULL, 0,
        "DeltaTime", HFILL }},
    { &hf_dsrc_rdRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_RequestorDescriptionRegional", HFILL }},
    { &hf_dsrc_rdRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_rpvPosition,
      { "position", "dsrc.position_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Position3D", HFILL }},
    { &hf_dsrc_heading,
      { "heading", "dsrc.heading",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Angle", HFILL }},
    { &hf_dsrc_rpvSpeed,
      { "speed", "dsrc.speed_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TransmissionAndSpeed", HFILL }},
    { &hf_dsrc_role,
      { "role", "dsrc.role",
        FT_UINT32, BASE_DEC, VALS(dsrc_BasicVehicleRole_vals), 0,
        "BasicVehicleRole", HFILL }},
    { &hf_dsrc_subrole,
      { "subrole", "dsrc.subrole",
        FT_UINT32, BASE_DEC, VALS(dsrc_RequestSubRole_vals), 0,
        "RequestSubRole", HFILL }},
    { &hf_dsrc_rtRequest,
      { "request", "dsrc.request",
        FT_UINT32, BASE_DEC, VALS(dsrc_RequestImportanceLevel_vals), 0,
        "RequestImportanceLevel", HFILL }},
    { &hf_dsrc_iso3883,
      { "iso3883", "dsrc.iso3883",
        FT_UINT32, BASE_DEC, VALS(erivdm_Iso3833VehicleType_vals), 0,
        "Iso3833VehicleType", HFILL }},
    { &hf_dsrc_hpmsType,
      { "hpmsType", "dsrc.hpmsType",
        FT_UINT32, BASE_DEC, VALS(dsrc_VehicleType_vals), 0,
        "VehicleType", HFILL }},
    { &hf_dsrc_rtRegional,
      { "regional", "dsrc.regional_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RegionalExtension", HFILL }},
    { &hf_dsrc_scaId,
      { "id", "dsrc.id",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RestrictionClassID", HFILL }},
    { &hf_dsrc_users,
      { "users", "dsrc.users",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RestrictionUserTypeList", HFILL }},
    { &hf_dsrc_RestrictionClassList_item,
      { "RestrictionClassAssignment", "dsrc.RestrictionClassAssignment_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_RestrictionUserTypeList_item,
      { "RestrictionUserType", "dsrc.RestrictionUserType",
        FT_UINT32, BASE_DEC, VALS(dsrc_RestrictionUserType_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_basicType,
      { "basicType", "dsrc.basicType",
        FT_UINT32, BASE_DEC, VALS(dsrc_RestrictionAppliesTo_vals), 0,
        "RestrictionAppliesTo", HFILL }},
    { &hf_dsrc_rutRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_RestrictionUserTypeRegional", HFILL }},
    { &hf_dsrc_rutRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_RoadLaneSetList_item,
      { "GenericLane", "dsrc.GenericLane_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_rsrId,
      { "id", "dsrc.id",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RoadSegmentID", HFILL }},
    { &hf_dsrc_rsId,
      { "id", "dsrc.id_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RoadSegmentReferenceID", HFILL }},
    { &hf_dsrc_roadLaneSet,
      { "roadLaneSet", "dsrc.roadLaneSet",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RoadLaneSetList", HFILL }},
    { &hf_dsrc_rsRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_RoadSegmentRegional", HFILL }},
    { &hf_dsrc_rsRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_RoadSegmentList_item,
      { "RoadSegment", "dsrc.RoadSegment_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_SegmentAttributeXYList_item,
      { "SegmentAttributeXY", "dsrc.SegmentAttributeXY",
        FT_UINT32, BASE_DEC, VALS(dsrc_SegmentAttributeXY_vals), 0,
        NULL, HFILL }},
    { &hf_dsrc_zone,
      { "zone", "dsrc.zone_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RegionalExtension", HFILL }},
    { &hf_dsrc_sriId,
      { "id", "dsrc.id",
        FT_UINT32, BASE_DEC, VALS(dsrc_VehicleID_vals), 0,
        "VehicleID", HFILL }},
    { &hf_dsrc_sriRequest,
      { "request", "dsrc.request",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RequestID", HFILL }},
    { &hf_dsrc_typeData,
      { "typeData", "dsrc.typeData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RequestorType", HFILL }},
    { &hf_dsrc_SignalRequestList_item,
      { "SignalRequestPackage", "dsrc.SignalRequestPackage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_srpRequest,
      { "request", "dsrc.request_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SignalRequest", HFILL }},
    { &hf_dsrc_minute,
      { "minute", "dsrc.minute",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MinuteOfTheYear", HFILL }},
    { &hf_dsrc_duration,
      { "duration", "dsrc.duration",
        FT_UINT32, BASE_DEC, NULL, 0,
        "DSecond", HFILL }},
    { &hf_dsrc_srpRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_SignalRequestPackageRegional", HFILL }},
    { &hf_dsrc_srpRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_srId,
      { "id", "dsrc.id_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "IntersectionReferenceID", HFILL }},
    { &hf_dsrc_requestID,
      { "requestID", "dsrc.requestID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_requestType,
      { "requestType", "dsrc.requestType",
        FT_UINT32, BASE_DEC, VALS(dsrc_PriorityRequestType_vals), 0,
        "PriorityRequestType", HFILL }},
    { &hf_dsrc_inBoundLane,
      { "inBoundLane", "dsrc.inBoundLane",
        FT_UINT32, BASE_DEC, VALS(dsrc_IntersectionAccessPoint_vals), 0,
        "IntersectionAccessPoint", HFILL }},
    { &hf_dsrc_outBoundLane,
      { "outBoundLane", "dsrc.outBoundLane",
        FT_UINT32, BASE_DEC, VALS(dsrc_IntersectionAccessPoint_vals), 0,
        "IntersectionAccessPoint", HFILL }},
    { &hf_dsrc_srRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_SignalRequestRegional", HFILL }},
    { &hf_dsrc_srRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_SignalStatusList_item,
      { "SignalStatus", "dsrc.SignalStatus_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_SignalStatusPackageList_item,
      { "SignalStatusPackage", "dsrc.SignalStatusPackage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_requester,
      { "requester", "dsrc.requester_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SignalRequesterInfo", HFILL }},
    { &hf_dsrc_inboundOn,
      { "inboundOn", "dsrc.inboundOn",
        FT_UINT32, BASE_DEC, VALS(dsrc_IntersectionAccessPoint_vals), 0,
        "IntersectionAccessPoint", HFILL }},
    { &hf_dsrc_outboundOn,
      { "outboundOn", "dsrc.outboundOn",
        FT_UINT32, BASE_DEC, VALS(dsrc_IntersectionAccessPoint_vals), 0,
        "IntersectionAccessPoint", HFILL }},
    { &hf_dsrc_sspStatus,
      { "status", "dsrc.status",
        FT_UINT32, BASE_DEC, VALS(dsrc_PrioritizationResponseStatus_vals), 0,
        "PrioritizationResponseStatus", HFILL }},
    { &hf_dsrc_sspRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_SignalStatusPackageRegional", HFILL }},
    { &hf_dsrc_sspRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_ssId,
      { "id", "dsrc.id_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "IntersectionReferenceID", HFILL }},
    { &hf_dsrc_sigStatus,
      { "sigStatus", "dsrc.sigStatus",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignalStatusPackageList", HFILL }},
    { &hf_dsrc_ssRegional,
      { "regional", "dsrc.regional",
        FT_UINT32, BASE_DEC, NULL, 0,
        "T_SignalStatusRegional", HFILL }},
    { &hf_dsrc_ssRegional_item,
      { "RegionalExtension", "dsrc.RegionalExtension_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_SpeedLimitList_item,
      { "RegulatorySpeedLimit", "dsrc.RegulatorySpeedLimit_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_startTime,
      { "startTime", "dsrc.startTime",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TimeMark", HFILL }},
    { &hf_dsrc_minEndTime,
      { "minEndTime", "dsrc.minEndTime",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TimeMark", HFILL }},
    { &hf_dsrc_maxEndTime,
      { "maxEndTime", "dsrc.maxEndTime",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TimeMark", HFILL }},
    { &hf_dsrc_likelyTime,
      { "likelyTime", "dsrc.likelyTime",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TimeMark", HFILL }},
    { &hf_dsrc_tcdConfidence,
      { "confidence", "dsrc.confidence",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TimeIntervalConfidence", HFILL }},
    { &hf_dsrc_nextTime,
      { "nextTime", "dsrc.nextTime",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TimeMark", HFILL }},
    { &hf_dsrc_transmisson,
      { "transmisson", "dsrc.transmisson",
        FT_UINT32, BASE_DEC, VALS(dsrc_TransmissionState_vals), 0,
        "TransmissionState", HFILL }},
    { &hf_dsrc_tasSpeed,
      { "speed", "dsrc.speed",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Velocity", HFILL }},
    { &hf_dsrc_entityID,
      { "entityID", "dsrc.entityID",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TemporaryID", HFILL }},
    { &hf_dsrc_stationID,
      { "stationID", "dsrc.stationID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_dsrc_LaneSharing_overlappingLaneDescriptionProvided,
      { "overlappingLaneDescriptionProvided", "dsrc.overlappingLaneDescriptionProvided",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneSharing_multipleLanesTreatedAsOneLane,
      { "multipleLanesTreatedAsOneLane", "dsrc.multipleLanesTreatedAsOneLane",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_LaneSharing_otherNonMotorizedTrafficTypes,
      { "otherNonMotorizedTrafficTypes", "dsrc.otherNonMotorizedTrafficTypes",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_LaneSharing_individualMotorizedVehicleTraffic,
      { "individualMotorizedVehicleTraffic", "dsrc.individualMotorizedVehicleTraffic",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_LaneSharing_busVehicleTraffic,
      { "busVehicleTraffic", "dsrc.busVehicleTraffic",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_LaneSharing_taxiVehicleTraffic,
      { "taxiVehicleTraffic", "dsrc.taxiVehicleTraffic",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_dsrc_LaneSharing_pedestriansTraffic,
      { "pedestriansTraffic", "dsrc.pedestriansTraffic",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_dsrc_LaneSharing_cyclistVehicleTraffic,
      { "cyclistVehicleTraffic", "dsrc.cyclistVehicleTraffic",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_dsrc_LaneSharing_trackedVehicleTraffic,
      { "trackedVehicleTraffic", "dsrc.trackedVehicleTraffic",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneSharing_pedestrianTraffic,
      { "pedestrianTraffic", "dsrc.pedestrianTraffic",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_maneuverStraightAllowed,
      { "maneuverStraightAllowed", "dsrc.maneuverStraightAllowed",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_maneuverLeftAllowed,
      { "maneuverLeftAllowed", "dsrc.maneuverLeftAllowed",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_maneuverRightAllowed,
      { "maneuverRightAllowed", "dsrc.maneuverRightAllowed",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_maneuverUTurnAllowed,
      { "maneuverUTurnAllowed", "dsrc.maneuverUTurnAllowed",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_maneuverLeftTurnOnRedAllowed,
      { "maneuverLeftTurnOnRedAllowed", "dsrc.maneuverLeftTurnOnRedAllowed",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_maneuverRightTurnOnRedAllowed,
      { "maneuverRightTurnOnRedAllowed", "dsrc.maneuverRightTurnOnRedAllowed",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_maneuverLaneChangeAllowed,
      { "maneuverLaneChangeAllowed", "dsrc.maneuverLaneChangeAllowed",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_maneuverNoStoppingAllowed,
      { "maneuverNoStoppingAllowed", "dsrc.maneuverNoStoppingAllowed",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_yieldAllwaysRequired,
      { "yieldAllwaysRequired", "dsrc.yieldAllwaysRequired",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_goWithHalt,
      { "goWithHalt", "dsrc.goWithHalt",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_caution,
      { "caution", "dsrc.caution",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_AllowedManeuvers_reserved1,
      { "reserved1", "dsrc.reserved1",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_manualControlIsEnabled,
      { "manualControlIsEnabled", "dsrc.manualControlIsEnabled",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_stopTimeIsActivated,
      { "stopTimeIsActivated", "dsrc.stopTimeIsActivated",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_failureFlash,
      { "failureFlash", "dsrc.failureFlash",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_preemptIsActive,
      { "preemptIsActive", "dsrc.preemptIsActive",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_signalPriorityIsActive,
      { "signalPriorityIsActive", "dsrc.signalPriorityIsActive",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_fixedTimeOperation,
      { "fixedTimeOperation", "dsrc.fixedTimeOperation",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_trafficDependentOperation,
      { "trafficDependentOperation", "dsrc.trafficDependentOperation",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_standbyOperation,
      { "standbyOperation", "dsrc.standbyOperation",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_failureMode,
      { "failureMode", "dsrc.failureMode",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_off,
      { "off", "dsrc.off",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_recentMAPmessageUpdate,
      { "recentMAPmessageUpdate", "dsrc.recentMAPmessageUpdate",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_recentChangeInMAPassignedLanesIDsUsed,
      { "recentChangeInMAPassignedLanesIDsUsed", "dsrc.recentChangeInMAPassignedLanesIDsUsed",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_noValidMAPisAvailableAtThisTime,
      { "noValidMAPisAvailableAtThisTime", "dsrc.noValidMAPisAvailableAtThisTime",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_IntersectionStatusObject_noValidSPATisAvailableAtThisTime,
      { "noValidSPATisAvailableAtThisTime", "dsrc.noValidSPATisAvailableAtThisTime",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Barrier_median_RevocableLane,
      { "median-RevocableLane", "dsrc.median-RevocableLane",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Barrier_median,
      { "median", "dsrc.median",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Barrier_whiteLineHashing,
      { "whiteLineHashing", "dsrc.whiteLineHashing",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Barrier_stripedLines,
      { "stripedLines", "dsrc.stripedLines",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Barrier_doubleStripedLines,
      { "doubleStripedLines", "dsrc.doubleStripedLines",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Barrier_trafficCones,
      { "trafficCones", "dsrc.trafficCones",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Barrier_constructionBarrier,
      { "constructionBarrier", "dsrc.constructionBarrier",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Barrier_trafficChannels,
      { "trafficChannels", "dsrc.trafficChannels",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Barrier_lowCurbs,
      { "lowCurbs", "dsrc.lowCurbs",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Barrier_highCurbs,
      { "highCurbs", "dsrc.highCurbs",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Bike_bikeRevocableLane,
      { "bikeRevocableLane", "dsrc.bikeRevocableLane",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Bike_pedestrianUseAllowed,
      { "pedestrianUseAllowed", "dsrc.pedestrianUseAllowed",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Bike_isBikeFlyOverLane,
      { "isBikeFlyOverLane", "dsrc.isBikeFlyOverLane",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Bike_fixedCycleTime,
      { "fixedCycleTime", "dsrc.fixedCycleTime",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Bike_biDirectionalCycleTimes,
      { "biDirectionalCycleTimes", "dsrc.biDirectionalCycleTimes",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Bike_isolatedByBarrier,
      { "isolatedByBarrier", "dsrc.isolatedByBarrier",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Bike_unsignalizedSegmentsPresent,
      { "unsignalizedSegmentsPresent", "dsrc.unsignalizedSegmentsPresent",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Crosswalk_crosswalkRevocableLane,
      { "crosswalkRevocableLane", "dsrc.crosswalkRevocableLane",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Crosswalk_bicyleUseAllowed,
      { "bicyleUseAllowed", "dsrc.bicyleUseAllowed",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Crosswalk_isXwalkFlyOverLane,
      { "isXwalkFlyOverLane", "dsrc.isXwalkFlyOverLane",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Crosswalk_fixedCycleTime,
      { "fixedCycleTime", "dsrc.fixedCycleTime",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Crosswalk_biDirectionalCycleTimes,
      { "biDirectionalCycleTimes", "dsrc.biDirectionalCycleTimes",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Crosswalk_hasPushToWalkButton,
      { "hasPushToWalkButton", "dsrc.hasPushToWalkButton",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Crosswalk_audioSupport,
      { "audioSupport", "dsrc.audioSupport",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Crosswalk_rfSignalRequestPresent,
      { "rfSignalRequestPresent", "dsrc.rfSignalRequestPresent",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Crosswalk_unsignalizedSegmentsPresent,
      { "unsignalizedSegmentsPresent", "dsrc.unsignalizedSegmentsPresent",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Parking_parkingRevocableLane,
      { "parkingRevocableLane", "dsrc.parkingRevocableLane",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Parking_parallelParkingInUse,
      { "parallelParkingInUse", "dsrc.parallelParkingInUse",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Parking_headInParkingInUse,
      { "headInParkingInUse", "dsrc.headInParkingInUse",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Parking_doNotParkZone,
      { "doNotParkZone", "dsrc.doNotParkZone",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Parking_parkingForBusUse,
      { "parkingForBusUse", "dsrc.parkingForBusUse",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Parking_parkingForTaxiUse,
      { "parkingForTaxiUse", "dsrc.parkingForTaxiUse",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Parking_noPublicParkingUse,
      { "noPublicParkingUse", "dsrc.noPublicParkingUse",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Sidewalk_sidewalk_RevocableLane,
      { "sidewalk-RevocableLane", "dsrc.sidewalk-RevocableLane",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Sidewalk_bicyleUseAllowed,
      { "bicyleUseAllowed", "dsrc.bicyleUseAllowed",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Sidewalk_isSidewalkFlyOverLane,
      { "isSidewalkFlyOverLane", "dsrc.isSidewalkFlyOverLane",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Sidewalk_walkBikes,
      { "walkBikes", "dsrc.walkBikes",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesRevocableLane,
      { "stripeToConnectingLanesRevocableLane", "dsrc.stripeToConnectingLanesRevocableLane",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Striping_stripeDrawOnLeft,
      { "stripeDrawOnLeft", "dsrc.stripeDrawOnLeft",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Striping_stripeDrawOnRight,
      { "stripeDrawOnRight", "dsrc.stripeDrawOnRight",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesLeft,
      { "stripeToConnectingLanesLeft", "dsrc.stripeToConnectingLanesLeft",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesRight,
      { "stripeToConnectingLanesRight", "dsrc.stripeToConnectingLanesRight",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Striping_stripeToConnectingLanesAhead,
      { "stripeToConnectingLanesAhead", "dsrc.stripeToConnectingLanesAhead",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_TrackedVehicle_spec_RevocableLane,
      { "spec-RevocableLane", "dsrc.spec-RevocableLane",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_TrackedVehicle_spec_commuterRailRoadTrack,
      { "spec-commuterRailRoadTrack", "dsrc.spec-commuterRailRoadTrack",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_TrackedVehicle_spec_lightRailRoadTrack,
      { "spec-lightRailRoadTrack", "dsrc.spec-lightRailRoadTrack",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_TrackedVehicle_spec_heavyRailRoadTrack,
      { "spec-heavyRailRoadTrack", "dsrc.spec-heavyRailRoadTrack",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_TrackedVehicle_spec_otherRailType,
      { "spec-otherRailType", "dsrc.spec-otherRailType",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Vehicle_isVehicleRevocableLane,
      { "isVehicleRevocableLane", "dsrc.isVehicleRevocableLane",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Vehicle_isVehicleFlyOverLane,
      { "isVehicleFlyOverLane", "dsrc.isVehicleFlyOverLane",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Vehicle_hovLaneUseOnly,
      { "hovLaneUseOnly", "dsrc.hovLaneUseOnly",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Vehicle_restrictedToBusUse,
      { "restrictedToBusUse", "dsrc.restrictedToBusUse",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Vehicle_restrictedToTaxiUse,
      { "restrictedToTaxiUse", "dsrc.restrictedToTaxiUse",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Vehicle_restrictedFromPublicUse,
      { "restrictedFromPublicUse", "dsrc.restrictedFromPublicUse",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Vehicle_hasIRbeaconCoverage,
      { "hasIRbeaconCoverage", "dsrc.hasIRbeaconCoverage",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_dsrc_LaneAttributes_Vehicle_permissionOnRequest,
      { "permissionOnRequest", "dsrc.permissionOnRequest",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_dsrc_LaneDirection_ingressPath,
      { "ingressPath", "dsrc.ingressPath",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_LaneDirection_egressPath,
      { "egressPath", "dsrc.egressPath",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_TransitVehicleStatus_loading,
      { "loading", "dsrc.loading",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_dsrc_TransitVehicleStatus_anADAuse,
      { "anADAuse", "dsrc.anADAuse",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_dsrc_TransitVehicleStatus_aBikeLoad,
      { "aBikeLoad", "dsrc.aBikeLoad",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_dsrc_TransitVehicleStatus_doorOpen,
      { "doorOpen", "dsrc.doorOpen",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_dsrc_TransitVehicleStatus_charging,
      { "charging", "dsrc.charging",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_dsrc_TransitVehicleStatus_atStopLine,
      { "atStopLine", "dsrc.atStopLine",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},

/* --- Module AddGrpC --- --- ---                                             */

    { &hf_AddGrpC_AddGrpC_ConnectionManeuverAssist_addGrpC_PDU,
      { "ConnectionManeuverAssist-addGrpC", "AddGrpC.ConnectionManeuverAssist_addGrpC_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_AddGrpC_ConnectionTrajectory_addGrpC_PDU,
      { "ConnectionTrajectory-addGrpC", "AddGrpC.ConnectionTrajectory_addGrpC_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_AddGrpC_Control_addGrpC_PDU,
      { "Control-addGrpC", "AddGrpC.Control_addGrpC_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_AddGrpC_IntersectionState_addGrpC_PDU,
      { "IntersectionState-addGrpC", "AddGrpC.IntersectionState_addGrpC_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_AddGrpC_MapData_addGrpC_PDU,
      { "MapData-addGrpC", "AddGrpC.MapData_addGrpC_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_AddGrpC_Position3D_addGrpC_PDU,
      { "Position3D-addGrpC", "AddGrpC.Position3D_addGrpC_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_AddGrpC_RestrictionUserType_addGrpC_PDU,
      { "RestrictionUserType-addGrpC", "AddGrpC.RestrictionUserType_addGrpC_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_AddGrpC_SignalStatusPackage_addGrpC_PDU,
      { "SignalStatusPackage-addGrpC", "AddGrpC.SignalStatusPackage_addGrpC_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_itsStationPositions,
      { "itsStationPositions", "AddGrpC.itsStationPositions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ItsStationPositionList", HFILL }},
    { &hf_AddGrpC_rsuGNSSOffset,
      { "rsuGNSSOffset", "AddGrpC.rsuGNSSOffset",
        FT_UINT32, BASE_DEC, VALS(dsrc_NodeOffsetPointXY_vals), 0,
        "NodeOffsetPointXY", HFILL }},
    { &hf_AddGrpC_nodes,
      { "nodes", "AddGrpC.nodes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "NodeSetXY", HFILL }},
    { &hf_AddGrpC_ptvRequest,
      { "ptvRequest", "AddGrpC.ptvRequest",
        FT_UINT32, BASE_DEC, VALS(AddGrpC_PtvRequestType_vals), 0,
        "PtvRequestType", HFILL }},
    { &hf_AddGrpC_activePrioritizations,
      { "activePrioritizations", "AddGrpC.activePrioritizations",
        FT_UINT32, BASE_DEC, NULL, 0,
        "PrioritizationResponseList", HFILL }},
    { &hf_AddGrpC_signalHeadLocations,
      { "signalHeadLocations", "AddGrpC.signalHeadLocations",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignalHeadLocationList", HFILL }},
    { &hf_AddGrpC_altitude,
      { "altitude", "AddGrpC.altitude_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_PrioritizationResponseList_item,
      { "PrioritizationResponse", "AddGrpC.PrioritizationResponse_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_stationID,
      { "stationID", "AddGrpC.stationID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_priorState,
      { "priorState", "AddGrpC.priorState",
        FT_UINT32, BASE_DEC, VALS(dsrc_PrioritizationResponseStatus_vals), 0,
        "PrioritizationResponseStatus", HFILL }},
    { &hf_AddGrpC_signalGroup,
      { "signalGroup", "AddGrpC.signalGroup",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SignalGroupID", HFILL }},
    { &hf_AddGrpC_emission,
      { "emission", "AddGrpC.emission",
        FT_UINT32, BASE_DEC, VALS(AddGrpC_EmissionType_vals), 0,
        "EmissionType", HFILL }},
    { &hf_AddGrpC_SignalHeadLocationList_item,
      { "SignalHeadLocation", "AddGrpC.SignalHeadLocation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_nodeXY,
      { "nodeXY", "AddGrpC.nodeXY",
        FT_UINT32, BASE_DEC, VALS(dsrc_NodeOffsetPointXY_vals), 0,
        "NodeOffsetPointXY", HFILL }},
    { &hf_AddGrpC_nodeZ,
      { "nodeZ", "AddGrpC.nodeZ",
        FT_INT32, BASE_DEC, VALS(its_DeltaAltitude_vals), 0,
        "DeltaAltitude", HFILL }},
    { &hf_AddGrpC_signalGroupID,
      { "signalGroupID", "AddGrpC.signalGroupID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_synchToSchedule,
      { "synchToSchedule", "AddGrpC.synchToSchedule",
        FT_INT32, BASE_DEC, NULL, 0,
        "DeltaTime", HFILL }},
    { &hf_AddGrpC_ItsStationPositionList_item,
      { "ItsStationPosition", "AddGrpC.ItsStationPosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_laneID,
      { "laneID", "AddGrpC.laneID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_AddGrpC_timeReference,
      { "timeReference", "AddGrpC.timeReference",
        FT_UINT32, BASE_DEC, VALS(AddGrpC_TimeReference_vals), 0,
        NULL, HFILL }},

/* --- Module CAM-PDU-Descriptions --- --- ---                                */

    { &hf_cam_cam_CoopAwareness_PDU,
      { "CoopAwareness", "cam.CoopAwareness_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_generationDeltaTime,
      { "generationDeltaTime", "cam.generationDeltaTime",
        FT_UINT32, BASE_DEC, VALS(cam_GenerationDeltaTime_vals), 0,
        NULL, HFILL }},
    { &hf_cam_camParameters,
      { "camParameters", "cam.camParameters_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_basicContainer,
      { "basicContainer", "cam.basicContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_highFrequencyContainer,
      { "highFrequencyContainer", "cam.highFrequencyContainer",
        FT_UINT32, BASE_DEC, VALS(cam_HighFrequencyContainer_vals), 0,
        NULL, HFILL }},
    { &hf_cam_lowFrequencyContainer,
      { "lowFrequencyContainer", "cam.lowFrequencyContainer",
        FT_UINT32, BASE_DEC, VALS(cam_LowFrequencyContainer_vals), 0,
        NULL, HFILL }},
    { &hf_cam_specialVehicleContainer,
      { "specialVehicleContainer", "cam.specialVehicleContainer",
        FT_UINT32, BASE_DEC, VALS(cam_SpecialVehicleContainer_vals), 0,
        NULL, HFILL }},
    { &hf_cam_basicVehicleContainerHighFrequency,
      { "basicVehicleContainerHighFrequency", "cam.basicVehicleContainerHighFrequency_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_rsuContainerHighFrequency,
      { "rsuContainerHighFrequency", "cam.rsuContainerHighFrequency_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_basicVehicleContainerLowFrequency,
      { "basicVehicleContainerLowFrequency", "cam.basicVehicleContainerLowFrequency_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_publicTransportContainer,
      { "publicTransportContainer", "cam.publicTransportContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_specialTransportContainer,
      { "specialTransportContainer", "cam.specialTransportContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_dangerousGoodsContainer,
      { "dangerousGoodsContainer", "cam.dangerousGoodsContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_roadWorksContainerBasic,
      { "roadWorksContainerBasic", "cam.roadWorksContainerBasic_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_rescueContainer,
      { "rescueContainer", "cam.rescueContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_emergencyContainer,
      { "emergencyContainer", "cam.emergencyContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_safetyCarContainer,
      { "safetyCarContainer", "cam.safetyCarContainer_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_stationType,
      { "stationType", "cam.stationType",
        FT_UINT32, BASE_DEC, VALS(its_StationType_vals), 0,
        NULL, HFILL }},
    { &hf_cam_referencePosition,
      { "referencePosition", "cam.referencePosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_heading,
      { "heading", "cam.heading_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_speed,
      { "speed", "cam.speed_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_driveDirection,
      { "driveDirection", "cam.driveDirection",
        FT_UINT32, BASE_DEC, VALS(its_DriveDirection_vals), 0,
        NULL, HFILL }},
    { &hf_cam_vehicleLength,
      { "vehicleLength", "cam.vehicleLength_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_vehicleWidth,
      { "vehicleWidth", "cam.vehicleWidth",
        FT_UINT32, BASE_DEC, VALS(its_VehicleWidth_vals), 0,
        NULL, HFILL }},
    { &hf_cam_longitudinalAcceleration,
      { "longitudinalAcceleration", "cam.longitudinalAcceleration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_curvature,
      { "curvature", "cam.curvature_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_curvatureCalculationMode,
      { "curvatureCalculationMode", "cam.curvatureCalculationMode",
        FT_UINT32, BASE_DEC, VALS(its_CurvatureCalculationMode_vals), 0,
        NULL, HFILL }},
    { &hf_cam_yawRate,
      { "yawRate", "cam.yawRate_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_accelerationControl,
      { "accelerationControl", "cam.accelerationControl",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_lanePosition,
      { "lanePosition", "cam.lanePosition",
        FT_INT32, BASE_DEC, VALS(its_LanePosition_vals), 0,
        NULL, HFILL }},
    { &hf_cam_steeringWheelAngle,
      { "steeringWheelAngle", "cam.steeringWheelAngle_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_lateralAcceleration,
      { "lateralAcceleration", "cam.lateralAcceleration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_verticalAcceleration,
      { "verticalAcceleration", "cam.verticalAcceleration_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_performanceClass,
      { "performanceClass", "cam.performanceClass",
        FT_UINT32, BASE_DEC, VALS(its_PerformanceClass_vals), 0,
        NULL, HFILL }},
    { &hf_cam_cenDsrcTollingZone,
      { "cenDsrcTollingZone", "cam.cenDsrcTollingZone_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_vehicleRole,
      { "vehicleRole", "cam.vehicleRole",
        FT_UINT32, BASE_DEC, VALS(its_VehicleRole_vals), 0,
        NULL, HFILL }},
    { &hf_cam_exteriorLights,
      { "exteriorLights", "cam.exteriorLights",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_pathHistory,
      { "pathHistory", "cam.pathHistory",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_embarkationStatus,
      { "embarkationStatus", "cam.embarkationStatus",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_ptActivation,
      { "ptActivation", "cam.ptActivation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_specialTransportType,
      { "specialTransportType", "cam.specialTransportType",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_lightBarSirenInUse,
      { "lightBarSirenInUse", "cam.lightBarSirenInUse",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_dangerousGoodsBasic,
      { "dangerousGoodsBasic", "cam.dangerousGoodsBasic",
        FT_UINT32, BASE_DEC, VALS(its_DangerousGoodsBasic_vals), 0,
        NULL, HFILL }},
    { &hf_cam_roadworksSubCauseCode,
      { "roadworksSubCauseCode", "cam.roadworksSubCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_RoadworksSubCauseCode_vals), 0,
        NULL, HFILL }},
    { &hf_cam_closedLanes,
      { "closedLanes", "cam.closedLanes_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_incidentIndication,
      { "incidentIndication", "cam.incidentIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CauseCode", HFILL }},
    { &hf_cam_emergencyPriority,
      { "emergencyPriority", "cam.emergencyPriority",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_cam_trafficRule,
      { "trafficRule", "cam.trafficRule",
        FT_UINT32, BASE_DEC, VALS(its_TrafficRule_vals), 0,
        NULL, HFILL }},
    { &hf_cam_speedLimit,
      { "speedLimit", "cam.speedLimit",
        FT_UINT32, BASE_DEC, VALS(its_SpeedLimit_vals), 0,
        NULL, HFILL }},
    { &hf_cam_protectedCommunicationZonesRSU,
      { "protectedCommunicationZonesRSU", "cam.protectedCommunicationZonesRSU",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},

/* --- Module DENM-PDU-Descriptions --- --- ---                               */

    { &hf_denm_denm_DecentralizedEnvironmentalNotificationMessage_PDU,
      { "DecentralizedEnvironmentalNotificationMessage", "denm.DecentralizedEnvironmentalNotificationMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_management,
      { "management", "denm.management_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ManagementContainer", HFILL }},
    { &hf_denm_situation,
      { "situation", "denm.situation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "SituationContainer", HFILL }},
    { &hf_denm_location,
      { "location", "denm.location_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "LocationContainer", HFILL }},
    { &hf_denm_alacarte,
      { "alacarte", "denm.alacarte_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "AlacarteContainer", HFILL }},
    { &hf_denm_actionID,
      { "actionID", "denm.actionID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_detectionTime,
      { "detectionTime", "denm.detectionTime",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_denm_referenceTime,
      { "referenceTime", "denm.referenceTime",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_denm_termination,
      { "termination", "denm.termination",
        FT_UINT32, BASE_DEC, VALS(denm_Termination_vals), 0,
        NULL, HFILL }},
    { &hf_denm_eventPosition,
      { "eventPosition", "denm.eventPosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ReferencePosition", HFILL }},
    { &hf_denm_relevanceDistance,
      { "relevanceDistance", "denm.relevanceDistance",
        FT_UINT32, BASE_DEC, VALS(its_RelevanceDistance_vals), 0,
        NULL, HFILL }},
    { &hf_denm_relevanceTrafficDirection,
      { "relevanceTrafficDirection", "denm.relevanceTrafficDirection",
        FT_UINT32, BASE_DEC, VALS(its_RelevanceTrafficDirection_vals), 0,
        NULL, HFILL }},
    { &hf_denm_validityDuration,
      { "validityDuration", "denm.validityDuration",
        FT_UINT32, BASE_DEC, VALS(its_ValidityDuration_vals), 0,
        NULL, HFILL }},
    { &hf_denm_transmissionInterval,
      { "transmissionInterval", "denm.transmissionInterval",
        FT_UINT32, BASE_DEC, VALS(its_TransmissionInterval_vals), 0,
        NULL, HFILL }},
    { &hf_denm_stationType,
      { "stationType", "denm.stationType",
        FT_UINT32, BASE_DEC, VALS(its_StationType_vals), 0,
        NULL, HFILL }},
    { &hf_denm_informationQuality,
      { "informationQuality", "denm.informationQuality",
        FT_UINT32, BASE_DEC, VALS(its_InformationQuality_vals), 0,
        NULL, HFILL }},
    { &hf_denm_eventType,
      { "eventType", "denm.eventType_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CauseCode", HFILL }},
    { &hf_denm_linkedCause,
      { "linkedCause", "denm.linkedCause_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CauseCode", HFILL }},
    { &hf_denm_eventHistory,
      { "eventHistory", "denm.eventHistory",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_eventSpeed,
      { "eventSpeed", "denm.eventSpeed_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Speed", HFILL }},
    { &hf_denm_eventPositionHeading,
      { "eventPositionHeading", "denm.eventPositionHeading_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Heading", HFILL }},
    { &hf_denm_traces,
      { "traces", "denm.traces",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_roadType,
      { "roadType", "denm.roadType",
        FT_UINT32, BASE_DEC, VALS(its_RoadType_vals), 0,
        NULL, HFILL }},
    { &hf_denm_heightLonCarrLeft,
      { "heightLonCarrLeft", "denm.heightLonCarrLeft",
        FT_UINT32, BASE_DEC, VALS(its_HeightLonCarr_vals), 0,
        "HeightLonCarr", HFILL }},
    { &hf_denm_heightLonCarrRight,
      { "heightLonCarrRight", "denm.heightLonCarrRight",
        FT_UINT32, BASE_DEC, VALS(its_HeightLonCarr_vals), 0,
        "HeightLonCarr", HFILL }},
    { &hf_denm_posLonCarrLeft,
      { "posLonCarrLeft", "denm.posLonCarrLeft",
        FT_UINT32, BASE_DEC, VALS(its_PosLonCarr_vals), 0,
        "PosLonCarr", HFILL }},
    { &hf_denm_posLonCarrRight,
      { "posLonCarrRight", "denm.posLonCarrRight",
        FT_UINT32, BASE_DEC, VALS(its_PosLonCarr_vals), 0,
        "PosLonCarr", HFILL }},
    { &hf_denm_positionOfPillars,
      { "positionOfPillars", "denm.positionOfPillars",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_posCentMass,
      { "posCentMass", "denm.posCentMass",
        FT_UINT32, BASE_DEC, VALS(its_PosCentMass_vals), 0,
        NULL, HFILL }},
    { &hf_denm_wheelBaseVehicle,
      { "wheelBaseVehicle", "denm.wheelBaseVehicle",
        FT_UINT32, BASE_DEC, VALS(its_WheelBaseVehicle_vals), 0,
        NULL, HFILL }},
    { &hf_denm_turningRadius,
      { "turningRadius", "denm.turningRadius",
        FT_UINT32, BASE_DEC, VALS(its_TurningRadius_vals), 0,
        NULL, HFILL }},
    { &hf_denm_posFrontAx,
      { "posFrontAx", "denm.posFrontAx",
        FT_UINT32, BASE_DEC, VALS(its_PosFrontAx_vals), 0,
        NULL, HFILL }},
    { &hf_denm_positionOfOccupants,
      { "positionOfOccupants", "denm.positionOfOccupants",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_vehicleMass,
      { "vehicleMass", "denm.vehicleMass",
        FT_UINT32, BASE_DEC, VALS(its_VehicleMass_vals), 0,
        NULL, HFILL }},
    { &hf_denm_requestResponseIndication,
      { "requestResponseIndication", "denm.requestResponseIndication",
        FT_UINT32, BASE_DEC, VALS(its_RequestResponseIndication_vals), 0,
        NULL, HFILL }},
    { &hf_denm_lightBarSirenInUse,
      { "lightBarSirenInUse", "denm.lightBarSirenInUse",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_closedLanes,
      { "closedLanes", "denm.closedLanes_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_restriction,
      { "restriction", "denm.restriction",
        FT_UINT32, BASE_DEC, NULL, 0,
        "RestrictedTypes", HFILL }},
    { &hf_denm_speedLimit,
      { "speedLimit", "denm.speedLimit",
        FT_UINT32, BASE_DEC, VALS(its_SpeedLimit_vals), 0,
        NULL, HFILL }},
    { &hf_denm_incidentIndication,
      { "incidentIndication", "denm.incidentIndication_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CauseCode", HFILL }},
    { &hf_denm_recommendedPath,
      { "recommendedPath", "denm.recommendedPath",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ItineraryPath", HFILL }},
    { &hf_denm_startingPointSpeedLimit,
      { "startingPointSpeedLimit", "denm.startingPointSpeedLimit_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "DeltaReferencePosition", HFILL }},
    { &hf_denm_trafficFlowRule,
      { "trafficFlowRule", "denm.trafficFlowRule",
        FT_UINT32, BASE_DEC, VALS(its_TrafficRule_vals), 0,
        "TrafficRule", HFILL }},
    { &hf_denm_referenceDenms,
      { "referenceDenms", "denm.referenceDenms",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_stationarySince,
      { "stationarySince", "denm.stationarySince",
        FT_UINT32, BASE_DEC, VALS(its_StationarySince_vals), 0,
        NULL, HFILL }},
    { &hf_denm_stationaryCause,
      { "stationaryCause", "denm.stationaryCause_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "CauseCode", HFILL }},
    { &hf_denm_carryingDangerousGoods,
      { "carryingDangerousGoods", "denm.carryingDangerousGoods_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "DangerousGoodsExtended", HFILL }},
    { &hf_denm_numberOfOccupants,
      { "numberOfOccupants", "denm.numberOfOccupants",
        FT_UINT32, BASE_DEC, VALS(its_NumberOfOccupants_vals), 0,
        NULL, HFILL }},
    { &hf_denm_vehicleIdentification,
      { "vehicleIdentification", "denm.vehicleIdentification_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_energyStorageType,
      { "energyStorageType", "denm.energyStorageType",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_denm_lanePosition,
      { "lanePosition", "denm.lanePosition",
        FT_INT32, BASE_DEC, VALS(its_LanePosition_vals), 0,
        NULL, HFILL }},
    { &hf_denm_impactReduction,
      { "impactReduction", "denm.impactReduction_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ImpactReductionContainer", HFILL }},
    { &hf_denm_externalTemperature,
      { "externalTemperature", "denm.externalTemperature",
        FT_INT32, BASE_DEC, VALS(its_Temperature_vals), 0,
        "Temperature", HFILL }},
    { &hf_denm_roadWorks,
      { "roadWorks", "denm.roadWorks_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "RoadWorksContainerExtended", HFILL }},
    { &hf_denm_positioningSolution,
      { "positioningSolution", "denm.positioningSolution",
        FT_UINT32, BASE_DEC, VALS(its_PositioningSolutionType_vals), 0,
        "PositioningSolutionType", HFILL }},
    { &hf_denm_stationaryVehicle,
      { "stationaryVehicle", "denm.stationaryVehicle_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "StationaryVehicleContainer", HFILL }},
    { &hf_denm_ReferenceDenms_item,
      { "ActionID", "denm.ActionID_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},

/* --- Module TIS-TPG-Transactions-Descriptions --- --- ---                   */

    { &hf_tistpg_tistpg_TisTpgTransaction_PDU,
      { "TisTpgTransaction", "tistpg.TisTpgTransaction",
        FT_UINT32, BASE_DEC, VALS(tistpg_TisTpgTransaction_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_drm,
      { "drm", "tistpg.drm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgDRM", HFILL }},
    { &hf_tistpg_snm,
      { "snm", "tistpg.snm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgSNM", HFILL }},
    { &hf_tistpg_trm,
      { "trm", "tistpg.trm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgTRM", HFILL }},
    { &hf_tistpg_tcm,
      { "tcm", "tistpg.tcm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgTCM", HFILL }},
    { &hf_tistpg_vdrm,
      { "vdrm", "tistpg.vdrm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgVDRM", HFILL }},
    { &hf_tistpg_vdpm,
      { "vdpm", "tistpg.vdpm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgVDPM", HFILL }},
    { &hf_tistpg_eofm,
      { "eofm", "tistpg.eofm_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgEOFM", HFILL }},
    { &hf_tistpg_drmManagement,
      { "management", "tistpg.management_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgDRM_Management", HFILL }},
    { &hf_tistpg_drmSituation,
      { "situation", "tistpg.situation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgDRM_Situation", HFILL }},
    { &hf_tistpg_drmLocation,
      { "location", "tistpg.location_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgDRM_Location", HFILL }},
    { &hf_tistpg_generationTime,
      { "generationTime", "tistpg.generationTime",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_tistpg_vehicleType,
      { "vehicleType", "tistpg.vehicleType",
        FT_UINT32, BASE_DEC, VALS(tistpg_UNVehicleClassifcation_vals), 0,
        "UNVehicleClassifcation", HFILL }},
    { &hf_tistpg_costumerContract,
      { "costumerContract", "tistpg.costumerContract",
        FT_STRING, BASE_NONE, NULL, 0,
        "CustomerContract", HFILL }},
    { &hf_tistpg_tisProfile,
      { "tisProfile", "tistpg.tisProfile",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_causeCode,
      { "causeCode", "tistpg.causeCode_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_vehiclePosition,
      { "vehiclePosition", "tistpg.vehiclePosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ReferencePosition", HFILL }},
    { &hf_tistpg_vehicleSpeed,
      { "vehicleSpeed", "tistpg.vehicleSpeed_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Speed", HFILL }},
    { &hf_tistpg_vehicleHeading,
      { "vehicleHeading", "tistpg.vehicleHeading_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "Heading", HFILL }},
    { &hf_tistpg_requestedPosition,
      { "requestedPosition", "tistpg.requestedPosition_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ReferencePosition", HFILL }},
    { &hf_tistpg_searchRange,
      { "searchRange", "tistpg.searchRange",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_searchCondition,
      { "searchCondition", "tistpg.searchCondition",
        FT_UINT32, BASE_DEC, VALS(tistpg_SearchCondition_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_snmManagement,
      { "management", "tistpg.management_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgSNM_Management", HFILL }},
    { &hf_tistpg_tpgContainer,
      { "tpgContainer", "tistpg.tpgContainer",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TpgNotifContainer", HFILL }},
    { &hf_tistpg_totalTpgStations,
      { "totalTpgStations", "tistpg.totalTpgStations",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_trmManagement,
      { "management", "tistpg.management_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgTRM_Management", HFILL }},
    { &hf_tistpg_trmSituation,
      { "situation", "tistpg.situation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgTRM_Situation", HFILL }},
    { &hf_tistpg_trmLocation,
      { "location", "tistpg.location_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgTRM_Location", HFILL }},
    { &hf_tistpg_tpgStationID,
      { "tpgStationID", "tistpg.tpgStationID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "StationID", HFILL }},
    { &hf_tistpg_reservationStatus,
      { "reservationStatus", "tistpg.reservationStatus",
        FT_UINT32, BASE_DEC, VALS(tistpg_ReservationStatus_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_costumercontract,
      { "costumercontract", "tistpg.costumercontract",
        FT_STRING, BASE_NONE, NULL, 0,
        "CustomerContract", HFILL }},
    { &hf_tistpg_reservationID,
      { "reservationID", "tistpg.reservationID",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_estArrivalTime,
      { "estArrivalTime", "tistpg.estArrivalTime",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_tistpg_proposedPairingID,
      { "proposedPairingID", "tistpg.proposedPairingID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "PairingID", HFILL }},
    { &hf_tistpg_tcmManagement,
      { "management", "tistpg.management_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgTCM_Management", HFILL }},
    { &hf_tistpg_tcmSituation,
      { "situation", "tistpg.situation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgTCM_Situation", HFILL }},
    { &hf_tistpg_tcmLocation,
      { "location", "tistpg.location_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgTCM_Location", HFILL }},
    { &hf_tistpg_reservedTpg,
      { "reservedTpg", "tistpg.reservedTpg",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_1_65535", HFILL }},
    { &hf_tistpg_tpgAutomationLevel,
      { "tpgAutomationLevel", "tistpg.tpgAutomationLevel",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TpgAutomation", HFILL }},
    { &hf_tistpg_pairingID,
      { "pairingID", "tistpg.pairingID",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_reservationTimeLimit,
      { "reservationTimeLimit", "tistpg.reservationTimeLimit",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_tistpg_cancellationCondition,
      { "cancellationCondition", "tistpg.cancellationCondition",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_tpgLocation,
      { "tpgLocation", "tistpg.tpgLocation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ReferencePosition", HFILL }},
    { &hf_tistpg_address,
      { "address", "tistpg.address",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String_SIZE_1_128", HFILL }},
    { &hf_tistpg_vdrmManagement,
      { "management", "tistpg.management_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgVDRM_Management", HFILL }},
    { &hf_tistpg_fillingStatus,
      { "fillingStatus", "tistpg.fillingStatus",
        FT_UINT32, BASE_DEC, VALS(tistpg_FillingStatus_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_automationLevel,
      { "automationLevel", "tistpg.automationLevel",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TpgAutomation", HFILL }},
    { &hf_tistpg_vdpmManagement,
      { "management", "tistpg.management_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgVDPM_Management", HFILL }},
    { &hf_tistpg_placardTable,
      { "placardTable", "tistpg.placardTable",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_vehicleSpecificData,
      { "vehicleSpecificData", "tistpg.vehicleSpecificData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_language,
      { "language", "tistpg.language",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_tyreTempCondition,
      { "tyreTempCondition", "tistpg.tyreTempCondition",
        FT_UINT32, BASE_DEC, VALS(tistpg_TyreTempCondition_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_currentVehicleConfiguration,
      { "currentVehicleConfiguration", "tistpg.currentVehicleConfiguration",
        FT_BYTES, BASE_NONE, NULL, 0,
        "PressureConfiguration", HFILL }},
    { &hf_tistpg_frontLeftTyreData,
      { "frontLeftTyreData", "tistpg.frontLeftTyreData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TyreData", HFILL }},
    { &hf_tistpg_frontRightTyreData,
      { "frontRightTyreData", "tistpg.frontRightTyreData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TyreData", HFILL }},
    { &hf_tistpg_rearLeftTyreData,
      { "rearLeftTyreData", "tistpg.rearLeftTyreData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TyreData", HFILL }},
    { &hf_tistpg_rearRightTyreData,
      { "rearRightTyreData", "tistpg.rearRightTyreData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TyreData", HFILL }},
    { &hf_tistpg_spareTyreData,
      { "spareTyreData", "tistpg.spareTyreData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TyreData", HFILL }},
    { &hf_tistpg_eofmManagement,
      { "management", "tistpg.management_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "TisTpgEOFM_Management", HFILL }},
    { &hf_tistpg_numberOfAppliedPressure,
      { "numberOfAppliedPressure", "tistpg.numberOfAppliedPressure",
        FT_UINT32, BASE_DEC, VALS(tistpg_NumberOfAppliedPressure_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_appliedTyrePressures,
      { "appliedTyrePressures", "tistpg.appliedTyrePressures",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_PlacardTable_item,
      { "TyreSetVariant", "tistpg.TyreSetVariant_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_variantID,
      { "variantID", "tistpg.variantID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "TyreSetVariantID", HFILL }},
    { &hf_tistpg_frontAxleDimension,
      { "frontAxleDimension", "tistpg.frontAxleDimension",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TyreSidewallInformation", HFILL }},
    { &hf_tistpg_rearAxleDimension,
      { "rearAxleDimension", "tistpg.rearAxleDimension",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TyreSidewallInformation", HFILL }},
    { &hf_tistpg_pressureVariantsList,
      { "pressureVariantsList", "tistpg.pressureVariantsList",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_PressureVariantsList_item,
      { "PressureVariant", "tistpg.PressureVariant_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_pressureConfiguration,
      { "pressureConfiguration", "tistpg.pressureConfiguration",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_frontAxlePressure,
      { "frontAxlePressure", "tistpg.frontAxlePressure",
        FT_UINT32, BASE_DEC, VALS(tistpg_AxlePlacardPressure_vals), 0,
        "AxlePlacardPressure", HFILL }},
    { &hf_tistpg_rearAxlePressure,
      { "rearAxlePressure", "tistpg.rearAxlePressure",
        FT_UINT32, BASE_DEC, VALS(tistpg_AxlePlacardPressure_vals), 0,
        "AxlePlacardPressure", HFILL }},
    { &hf_tistpg_currentTyrePressure,
      { "currentTyrePressure", "tistpg.currentTyrePressure",
        FT_UINT32, BASE_DEC, VALS(tistpg_T_currentTyrePressure_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_tyrePressureValue,
      { "tyrePressureValue", "tistpg.tyrePressureValue",
        FT_UINT32, BASE_DEC, VALS(tistpg_TyrePressure_vals), 0,
        "TyrePressure", HFILL }},
    { &hf_tistpg_unavailable,
      { "unavailable", "tistpg.unavailable_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_tyreSidewallInformation,
      { "tyreSidewallInformation", "tistpg.tyreSidewallInformation",
        FT_UINT32, BASE_DEC, VALS(tistpg_T_tyreSidewallInformation_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_tyreSidewallInformationValue,
      { "tyreSidewallInformationValue", "tistpg.tyreSidewallInformationValue",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TyreSidewallInformation", HFILL }},
    { &hf_tistpg_currentInsideAirTemperature,
      { "currentInsideAirTemperature", "tistpg.currentInsideAirTemperature",
        FT_UINT32, BASE_DEC, VALS(tistpg_T_currentInsideAirTemperature_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_tyreAirTemperatureValue,
      { "tyreAirTemperatureValue", "tistpg.tyreAirTemperatureValue",
        FT_UINT32, BASE_DEC, VALS(tistpg_TyreAirTemperature_vals), 0,
        "TyreAirTemperature", HFILL }},
    { &hf_tistpg_recommendedTyrePressure,
      { "recommendedTyrePressure", "tistpg.recommendedTyrePressure",
        FT_UINT32, BASE_DEC, VALS(tistpg_T_recommendedTyrePressure_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_axlePlacardPressureValue,
      { "axlePlacardPressureValue", "tistpg.axlePlacardPressureValue",
        FT_UINT32, BASE_DEC, VALS(tistpg_AxlePlacardPressure_vals), 0,
        "AxlePlacardPressure", HFILL }},
    { &hf_tistpg_tin,
      { "tin", "tistpg.tin",
        FT_UINT32, BASE_DEC, VALS(tistpg_T_tin_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_tinValue,
      { "tinValue", "tistpg.tinValue",
        FT_BYTES, BASE_NONE, NULL, 0,
        "TIN", HFILL }},
    { &hf_tistpg_sensorState,
      { "sensorState", "tistpg.sensorState",
        FT_UINT32, BASE_DEC, VALS(tistpg_T_sensorState_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_sensorStateValue,
      { "sensorStateValue", "tistpg.sensorStateValue",
        FT_UINT32, BASE_DEC, VALS(tistpg_SensorState_vals), 0,
        "SensorState", HFILL }},
    { &hf_tistpg_tpgNumber,
      { "tpgNumber", "tistpg.tpgNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_tpgProvider,
      { "tpgProvider", "tistpg.tpgProvider",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_accessibility,
      { "accessibility", "tistpg.accessibility",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_phoneNumber,
      { "phoneNumber", "tistpg.phoneNumber",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_digitalMap,
      { "digitalMap", "tistpg.digitalMap",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_openingDaysHours,
      { "openingDaysHours", "tistpg.openingDaysHours",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_bookingInfo,
      { "bookingInfo", "tistpg.bookingInfo",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_availableTpgNumber,
      { "availableTpgNumber", "tistpg.availableTpgNumber",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_AppliedTyrePressures_item,
      { "AppliedTyrePressure", "tistpg.AppliedTyrePressure",
        FT_UINT32, BASE_DEC, VALS(tistpg_AppliedTyrePressure_vals), 0,
        NULL, HFILL }},
    { &hf_tistpg_TpgNotifContainer_item,
      { "TpgStationData", "tistpg.TpgStationData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_tistpg_TpgAutomation_fullAutomated,
      { "fullAutomated", "tistpg.fullAutomated",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_tistpg_TpgAutomation_semiAutomated,
      { "semiAutomated", "tistpg.semiAutomated",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_tistpg_TpgAutomation_manual,
      { "manual", "tistpg.manual",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_tistpg_TpgAutomation_reserved,
      { "reserved", "tistpg.reserved",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_tistpg_TisProfile_reserved,
      { "reserved", "tistpg.reserved",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_tistpg_TisProfile_profileOne,
      { "profileOne", "tistpg.profileOne",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_tistpg_TisProfile_profileTwo,
      { "profileTwo", "tistpg.profileTwo",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_tistpg_TisProfile_profileThree,
      { "profileThree", "tistpg.profileThree",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},

/* --- Module EVCSN-PDU-Descriptions --- --- ---                              */

    { &hf_evcsn_evcsn_EVChargingSpotNotificationPOIMessage_PDU,
      { "EVChargingSpotNotificationPOIMessage", "evcsn.EVChargingSpotNotificationPOIMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evcsn_poiHeader,
      { "poiHeader", "evcsn.poiHeader_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ItsPOIHeader", HFILL }},
    { &hf_evcsn_evcsnData,
      { "evcsnData", "evcsn.evcsnData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ItsEVCSNData", HFILL }},
    { &hf_evcsn_poiType,
      { "poiType", "evcsn.poiType",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_evcsn_timeStamp,
      { "timeStamp", "evcsn.timeStamp",
        FT_UINT64, BASE_DEC, NULL, 0,
        "TimestampIts", HFILL }},
    { &hf_evcsn_relayCapable,
      { "relayCapable", "evcsn.relayCapable",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_evcsn_totalNumberOfStations,
      { "totalNumberOfStations", "evcsn.totalNumberOfStations",
        FT_UINT32, BASE_DEC, NULL, 0,
        "NumberStations", HFILL }},
    { &hf_evcsn_chargingStationsData,
      { "chargingStationsData", "evcsn.chargingStationsData",
        FT_UINT32, BASE_DEC, NULL, 0,
        "SEQUENCE_SIZE_1_256_OF_ItsChargingStationData", HFILL }},
    { &hf_evcsn_chargingStationsData_item,
      { "ItsChargingStationData", "evcsn.ItsChargingStationData_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evcsn_chargingStationID,
      { "chargingStationID", "evcsn.chargingStationID",
        FT_UINT32, BASE_DEC, NULL, 0,
        "StationID", HFILL }},
    { &hf_evcsn_utilityDistributorId,
      { "utilityDistributorId", "evcsn.utilityDistributorId",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String_SIZE_1_32", HFILL }},
    { &hf_evcsn_providerID,
      { "providerID", "evcsn.providerID",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String_SIZE_1_32", HFILL }},
    { &hf_evcsn_chargingStationLocation,
      { "chargingStationLocation", "evcsn.chargingStationLocation_element",
        FT_NONE, BASE_NONE, NULL, 0,
        "ReferencePosition", HFILL }},
    { &hf_evcsn_address,
      { "address", "evcsn.address",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_evcsn_phoneNumber,
      { "phoneNumber", "evcsn.phoneNumber",
        FT_STRING, BASE_NONE, NULL, 0,
        "NumericString_SIZE_1_16", HFILL }},
    { &hf_evcsn_accessibility,
      { "accessibility", "evcsn.accessibility",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String_SIZE_1_32", HFILL }},
    { &hf_evcsn_digitalMap,
      { "digitalMap", "evcsn.digitalMap",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_evcsn_openingDaysHours,
      { "openingDaysHours", "evcsn.openingDaysHours",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_evcsn_pricing,
      { "pricing", "evcsn.pricing",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_evcsn_bookingContactInfo,
      { "bookingContactInfo", "evcsn.bookingContactInfo",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_evcsn_payment,
      { "payment", "evcsn.payment",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_evcsn_chargingSpotsAvailable,
      { "chargingSpotsAvailable", "evcsn.chargingSpotsAvailable",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ItsChargingSpots", HFILL }},
    { &hf_evcsn_ItsChargingSpots_item,
      { "ItsChargingSpotDataElements", "evcsn.ItsChargingSpotDataElements_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evcsn_type,
      { "type", "evcsn.type",
        FT_BYTES, BASE_NONE, NULL, 0,
        "ChargingSpotType", HFILL }},
    { &hf_evcsn_evEquipmentID,
      { "evEquipmentID", "evcsn.evEquipmentID",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_evcsn_typeOfReceptacle,
      { "typeOfReceptacle", "evcsn.typeOfReceptacle",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evcsn_energyAvailability,
      { "energyAvailability", "evcsn.energyAvailability",
        FT_STRING, BASE_NONE, NULL, 0,
        "UTF8String", HFILL }},
    { &hf_evcsn_parkingPlacesData,
      { "parkingPlacesData", "evcsn.parkingPlacesData",
        FT_UINT32, BASE_DEC, NULL, 0,
        NULL, HFILL }},
    { &hf_evcsn_ParkingPlacesData_item,
      { "SpotAvailability", "evcsn.SpotAvailability_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evcsn_maxWaitingTimeMinutes,
      { "maxWaitingTimeMinutes", "evcsn.maxWaitingTimeMinutes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "INTEGER_0_1400", HFILL }},
    { &hf_evcsn_blocking,
      { "blocking", "evcsn.blocking",
        FT_BOOLEAN, BASE_NONE, NULL, 0,
        "BOOLEAN", HFILL }},
    { &hf_evcsn_ChargingSpotType_standardChargeMode1,
      { "standardChargeMode1", "evcsn.standardChargeMode1",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_evcsn_ChargingSpotType_standardChargeMode2,
      { "standardChargeMode2", "evcsn.standardChargeMode2",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_evcsn_ChargingSpotType_standardOrFastChargeMode3,
      { "standardOrFastChargeMode3", "evcsn.standardOrFastChargeMode3",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_evcsn_ChargingSpotType_fastChargeWithExternalCharger,
      { "fastChargeWithExternalCharger", "evcsn.fastChargeWithExternalCharger",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_evcsn_ChargingSpotType_quickDrop,
      { "quickDrop", "evcsn.quickDrop",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_evcsn_ChargingSpotType_inductiveChargeWhileStationary,
      { "inductiveChargeWhileStationary", "evcsn.inductiveChargeWhileStationary",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_evcsn_ChargingSpotType_inductiveChargeWhileDriving,
      { "inductiveChargeWhileDriving", "evcsn.inductiveChargeWhileDriving",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},

/* --- Module EV-RechargingSpotReservation-PDU-Descriptions --- --- ---       */

    { &hf_evrsr_evrsr_EV_RSR_MessageBody_PDU,
      { "EV-RSR-MessageBody", "evrsr.EV_RSR_MessageBody",
        FT_UINT32, BASE_DEC, VALS(evrsr_EV_RSR_MessageBody_vals), 0,
        NULL, HFILL }},
    { &hf_evrsr_preReservationRequestMessage,
      { "preReservationRequestMessage", "evrsr.preReservationRequestMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_preReservationResponseMessage,
      { "preReservationResponseMessage", "evrsr.preReservationResponseMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_reservationRequestMessage,
      { "reservationRequestMessage", "evrsr.reservationRequestMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_reservationResponseMessage,
      { "reservationResponseMessage", "evrsr.reservationResponseMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_cancellationRequestMessage,
      { "cancellationRequestMessage", "evrsr.cancellationRequestMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_cancellationResponseMessage,
      { "cancellationResponseMessage", "evrsr.cancellationResponseMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_updateRequestMessage,
      { "updateRequestMessage", "evrsr.updateRequestMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_updateResponseMessage,
      { "updateResponseMessage", "evrsr.updateResponseMessage_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_evse_ID,
      { "evse-ID", "evrsr.evse_ID",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_arrivalTime,
      { "arrivalTime", "evrsr.arrivalTime",
        FT_INT32, BASE_DEC, VALS(evrsr_TimestampUTC_vals), 0,
        "TimestampUTC", HFILL }},
    { &hf_evrsr_departureTime,
      { "departureTime", "evrsr.departureTime",
        FT_INT32, BASE_DEC, VALS(evrsr_TimestampUTC_vals), 0,
        "TimestampUTC", HFILL }},
    { &hf_evrsr_rechargingType,
      { "rechargingType", "evrsr.rechargingType_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_batteryType,
      { "batteryType", "evrsr.batteryType",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_preReservation_ID,
      { "preReservation-ID", "evrsr.preReservation_ID",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_availabilityStatus,
      { "availabilityStatus", "evrsr.availabilityStatus",
        FT_UINT32, BASE_DEC, VALS(evrsr_AvailabilityStatus_vals), 0,
        NULL, HFILL }},
    { &hf_evrsr_preReservationExpirationTime,
      { "preReservationExpirationTime", "evrsr.preReservationExpirationTime",
        FT_INT32, BASE_DEC, VALS(evrsr_TimestampUTC_vals), 0,
        "TimestampUTC", HFILL }},
    { &hf_evrsr_supportedPaymentTypes,
      { "supportedPaymentTypes", "evrsr.supportedPaymentTypes",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_currentTime,
      { "currentTime", "evrsr.currentTime",
        FT_INT32, BASE_DEC, VALS(evrsr_TimestampUTC_vals), 0,
        "TimestampUTC", HFILL }},
    { &hf_evrsr_eAmount,
      { "eAmount", "evrsr.eAmount",
        FT_UINT32, BASE_DEC, VALS(evrsr_EAmount_vals), 0,
        NULL, HFILL }},
    { &hf_evrsr_eAmountMin,
      { "eAmountMin", "evrsr.eAmountMin",
        FT_UINT32, BASE_DEC, VALS(evrsr_EAmount_vals), 0,
        "EAmount", HFILL }},
    { &hf_evrsr_paymentType,
      { "paymentType", "evrsr.paymentType",
        FT_UINT32, BASE_DEC, VALS(evrsr_PaymentType_vals), 0,
        NULL, HFILL }},
    { &hf_evrsr_payment_ID,
      { "payment-ID", "evrsr.payment_ID",
        FT_UINT32, BASE_DEC, VALS(evrsr_Payment_ID_vals), 0,
        NULL, HFILL }},
    { &hf_evrsr_secondPayment_ID,
      { "secondPayment-ID", "evrsr.secondPayment_ID",
        FT_UINT32, BASE_DEC, VALS(evrsr_Payment_ID_vals), 0,
        "Payment_ID", HFILL }},
    { &hf_evrsr_pairing_ID,
      { "pairing-ID", "evrsr.pairing_ID",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_reservationResponseCode,
      { "reservationResponseCode", "evrsr.reservationResponseCode",
        FT_UINT32, BASE_DEC, VALS(evrsr_ReservationResponseCode_vals), 0,
        NULL, HFILL }},
    { &hf_evrsr_reservation_ID,
      { "reservation-ID", "evrsr.reservation_ID",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_reservation_Password,
      { "reservation-Password", "evrsr.reservation_Password",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_stationDetails,
      { "stationDetails", "evrsr.stationDetails",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_chargingSpotLabel,
      { "chargingSpotLabel", "evrsr.chargingSpotLabel",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_expirationTime,
      { "expirationTime", "evrsr.expirationTime",
        FT_INT32, BASE_DEC, VALS(evrsr_TimestampUTC_vals), 0,
        "TimestampUTC", HFILL }},
    { &hf_evrsr_freeCancelTimeLimit,
      { "freeCancelTimeLimit", "evrsr.freeCancelTimeLimit",
        FT_INT32, BASE_DEC, VALS(evrsr_TimestampUTC_vals), 0,
        "TimestampUTC", HFILL }},
    { &hf_evrsr_cancellationResponseCode,
      { "cancellationResponseCode", "evrsr.cancellationResponseCode",
        FT_UINT32, BASE_DEC, VALS(evrsr_CancellationResponseCode_vals), 0,
        NULL, HFILL }},
    { &hf_evrsr_updatedArrivalTime,
      { "updatedArrivalTime", "evrsr.updatedArrivalTime",
        FT_INT32, BASE_DEC, VALS(evrsr_TimestampUTC_vals), 0,
        "TimestampUTC", HFILL }},
    { &hf_evrsr_updatedDepartureTime,
      { "updatedDepartureTime", "evrsr.updatedDepartureTime",
        FT_INT32, BASE_DEC, VALS(evrsr_TimestampUTC_vals), 0,
        "TimestampUTC", HFILL }},
    { &hf_evrsr_updateResponseCode,
      { "updateResponseCode", "evrsr.updateResponseCode",
        FT_UINT32, BASE_DEC, VALS(evrsr_UpdateResponseCode_vals), 0,
        NULL, HFILL }},
    { &hf_evrsr_contractID,
      { "contractID", "evrsr.contractID",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_externalIdentificationMeans,
      { "externalIdentificationMeans", "evrsr.externalIdentificationMeans",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_evrsr_rechargingMode,
      { "rechargingMode", "evrsr.rechargingMode",
        FT_UINT32, BASE_DEC, VALS(evrsr_RechargingMode_vals), 0,
        NULL, HFILL }},
    { &hf_evrsr_powerSource,
      { "powerSource", "evrsr.powerSource",
        FT_UINT32, BASE_DEC, VALS(evrsr_PowerSource_vals), 0,
        NULL, HFILL }},
    { &hf_evrsr_SupportedPaymentTypes_contract,
      { "contract", "evrsr.contract",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_evrsr_SupportedPaymentTypes_externalIdentification,
      { "externalIdentification", "evrsr.externalIdentification",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},

/*--- End of included file: packet-its-hfarr.c ---*/
#line 379 "./asn1/its/packet-its-template.c"

    { &hf_its_roadworksSubCauseCode,
      { "roadworksSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_RoadworksSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_postCrashSubCauseCode,
      { "postCrashSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_PostCrashSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_vehicleBreakdownSubCauseCode,
      { "vehicleBreakdownSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_VehicleBreakdownSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_dangerousSituationSubCauseCode,
      { "dangerousSituationSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_DangerousSituationSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_dangerousEndOfQueueSubCauseCode,
      { "dangerousEndOfQueueSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_DangerousEndOfQueueSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_rescueAndRecoveryWorkInProgressSubCauseCode,
      { "rescueAndRecoveryWorkInProgressSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_RescueAndRecoveryWorkInProgressSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_signalViolationSubCauseCode,
      { "signalViolationSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_SignalViolationSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_collisionRiskSubCauseCode,
      { "collisionRiskSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_CollisionRiskSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_hazardousLocation_AnimalOnTheRoadSubCauseCode,
      { "hazardousLocation_AnimalOnTheRoadSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_HazardousLocation_AnimalOnTheRoadSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_hazardousLocation_ObstacleOnTheRoadSubCauseCode,
      { "hazardousLocation_ObstacleOnTheRoadSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_HazardousLocation_ObstacleOnTheRoadSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_hazardousLocation_SurfaceConditionSubCauseCode,
      { "hazardousLocation_SurfaceConditionSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_HazardousLocation_SurfaceConditionSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_hazardousLocation_DangerousCurveSubCauseCode,
      { "hazardousLocation_DangerousCurveSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_HazardousLocation_DangerousCurveSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_emergencyVehicleApproachingSubCauseCode,
      { "emergencyVehicleApproachingSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_EmergencyVehicleApproachingSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_humanProblemSubCauseCode,
      { "humanProblemSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_HumanProblemSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_stationaryVehicleSubCauseCode,
      { "stationaryVehicleSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_StationaryVehicleSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_slowVehicleSubCauseCode,
      { "slowVehicleSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_SlowVehicleSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_adverseWeatherCondition_PrecipitationSubCauseCode,
      { "adverseWeatherCondition_PrecipitationSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_AdverseWeatherCondition_PrecipitationSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_adverseWeatherCondition_VisibilitySubCauseCode,
      { "adverseWeatherCondition_VisibilitySubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_AdverseWeatherCondition_VisibilitySubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_adverseWeatherCondition_AdhesionSubCauseCode,
      { "adverseWeatherCondition_AdhesionSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_AdverseWeatherCondition_AdhesionSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_adverseWeatherCondition_ExtremeWeatherConditionSubCauseCode,
      { "adverseWeatherCondition_ExtremeWeatherConditionSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_AdverseWeatherCondition_ExtremeWeatherConditionSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_wrongWayDrivingSubCauseCode,
      { "wrongWayDrivingSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_WrongWayDrivingSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_humanPresenceOnTheRoadSubCauseCode,
      { "humanPresenceOnTheRoadSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_HumanPresenceOnTheRoadSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_accidentSubCauseCode,
      { "accidentSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_AccidentSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},
    { &hf_its_trafficConditionSubCauseCode,
      { "trafficConditionSubCauseCode", "its.subCauseCode",
        FT_UINT32, BASE_DEC, VALS(its_TrafficConditionSubCauseCode_vals), 0,
        "SubCauseCodeType", HFILL }},

    /*
     * DENM SSP
     */
    { &hf_denmssp_version, { "Version", "its.ssp.denm.version", FT_UINT8, BASE_DEC, NULL, 0, NULL, HFILL }},
    { &hf_denmssp_flags, { "Allowed to sign", "its.ssp.denm.flags", FT_UINT24, BASE_HEX, NULL, 0, NULL, HFILL }},
    { &hf_denmssp_trafficCondition,
        { "trafficCondition",                     "its.denm.ssp.trafficCondition",
            FT_UINT24, BASE_DEC, NULL, 0x800000, NULL, HFILL }},
    { &hf_denmssp_accident,
        { "accident",                             "its.denm.ssp.accident",
            FT_UINT24, BASE_DEC, NULL, 0x400000, NULL, HFILL }},
    { &hf_denmssp_roadworks,
        { "roadworks",                            "its.denm.ssp.roadworks",
            FT_UINT24, BASE_DEC, NULL, 0x200000, NULL, HFILL }},
    { &hf_denmssp_adverseWeatherConditionAdhesion,
        { "adverseWeatherConditionAdhesion",      "its.denm.ssp.advWxConditionAdhesion",
            FT_UINT24, BASE_DEC, NULL, 0x100000, NULL, HFILL }},
    { &hf_denmssp_hazardousLocationSurfaceCondition,
        { "hazardousLocationSurfaceCondition",    "its.denm.ssp.hazLocationSurfaceCondition",
            FT_UINT24, BASE_DEC, NULL, 0x080000, NULL, HFILL }},
    { &hf_denmssp_hazardousLocationObstacleOnTheRoad,
        { "hazardousLocationObstacleOnTheRoad",   "its.denm.ssp.hazLocationObstacleOnTheRoad",
            FT_UINT24, BASE_DEC, NULL, 0x040000, NULL, HFILL }},
    { &hf_denmssp_hazardousLocationAnimalOnTheRoad,
        { "hazardousLocationAnimalOnTheRoad",     "its.denm.ssp.hazLocationAnimalOnTheRoad",
            FT_UINT24, BASE_DEC, NULL, 0x020000, NULL, HFILL }},
    { &hf_denmssp_humanPresenceOnTheRoad,
        { "humanPresenceOnTheRoad",               "its.denm.ssp.humanPresenceOnTheRoad",
            FT_UINT24, BASE_DEC, NULL, 0x010000, NULL, HFILL }},
    { &hf_denmssp_wrongWayDriving,
        { "wrongWayDriving",                      "its.denm.ssp.wrongWayDriving",
            FT_UINT24, BASE_DEC, NULL, 0x008000, NULL, HFILL }},
    { &hf_denmssp_rescueAndRecoveryWorkInProgress,
        { "rescueAndRecoveryWorkInProgress",      "its.denm.ssp.rescueAndRecoveryWorkInProgress",
            FT_UINT24, BASE_DEC, NULL, 0x004000, NULL, HFILL }},
    { &hf_denmssp_ExtremeWeatherCondition,
        { "ExtremeWeatherCondition",              "its.denm.ssp.ExtremeWxCondition",
            FT_UINT24, BASE_DEC, NULL, 0x002000, NULL, HFILL }},
    { &hf_denmssp_adverseWeatherConditionVisibility,
        { "adverseWeatherConditionVisibility",    "its.denm.ssp.advWxConditionVisibility",
            FT_UINT24, BASE_DEC, NULL, 0x001000, NULL, HFILL }},
    { &hf_denmssp_adverseWeatherConditionPrecipitation,
        { "adverseWeatherConditionPrecipitation", "its.denm.ssp.advWxConditionPrecipitation",
            FT_UINT24, BASE_DEC, NULL, 0x000800, NULL, HFILL }},
    { &hf_denmssp_slowVehicle,
        { "slowVehicle",                          "its.denm.ssp.slowVehicle",
            FT_UINT24, BASE_DEC, NULL, 0x000400, NULL, HFILL }},
    { &hf_denmssp_dangerousEndOfQueue,
        { "dangerousEndOfQueue",                  "its.denm.ssp.dangerousEndOfQueue",
            FT_UINT24, BASE_DEC, NULL, 0x000200, NULL, HFILL }},
    { &hf_denmssp_vehicleBreakdown,
        { "vehicleBreakdown",                     "its.denm.ssp.vehicleBreakdown",
            FT_UINT24, BASE_DEC, NULL, 0x000100, NULL, HFILL }},
    { &hf_denmssp_postCrash,
        { "postCrash",                            "its.denm.ssp.postCrash",
            FT_UINT24, BASE_DEC, NULL, 0x000080, NULL, HFILL }},
    { &hf_denmssp_humanProblem,
        { "humanProblem",                         "its.denm.ssp.humanProblem",
            FT_UINT24, BASE_DEC, NULL, 0x000040, NULL, HFILL }},
    { &hf_denmssp_stationaryVehicle,
        { "stationaryVehicle",                    "its.denm.ssp.stationaryVehicle",
            FT_UINT24, BASE_DEC, NULL, 0x000020, NULL, HFILL }},
    { &hf_denmssp_emergencyVehicleApproaching,
        { "emergencyVehicleApproaching",          "its.denm.ssp.emergencyVehicleApproaching",
            FT_UINT24, BASE_DEC, NULL, 0x000010, NULL, HFILL }},
    { &hf_denmssp_hazardousLocationDangerousCurve,
        { "hazardousLocationDangerousCurve",      "its.denm.ssp.hazLocationDangerousCurve",
            FT_UINT24, BASE_DEC, NULL, 0x000008, NULL, HFILL }},
    { &hf_denmssp_collisionRisk,
        { "collisionRisk",                        "its.denm.ssp.collisionRisk",
            FT_UINT24, BASE_DEC, NULL, 0x000004, NULL, HFILL }},
    { &hf_denmssp_signalViolation,
        { "signalViolation",                      "its.denm.ssp.signalViolation",
            FT_UINT24, BASE_DEC, NULL, 0x000002, NULL, HFILL }},
    { &hf_denmssp_dangerousSituation,
        { "dangerousSituation",                   "its.denm.ssp.dangerousSituation",
            FT_UINT24, BASE_DEC, NULL, 0x000001, NULL, HFILL }},

    /*
     * CAM SSP
     */
    { &hf_camssp_version, { "Version", "its.ssp.cam.version", FT_UINT8, BASE_DEC, NULL, 0, NULL, HFILL }},
    { &hf_camssp_flags, { "Allowed to sign", "its.ssp.cam.flags", FT_UINT16, BASE_HEX, NULL, 0, NULL, HFILL }},
    { &hf_camssp_cenDsrcTollingZone, { "cenDsrcTollingZone", "its.ssp.cam.cenDsrcTollingZone", FT_UINT16, BASE_DEC, NULL, 0x8000, NULL, HFILL }},
    { &hf_camssp_publicTransport, { "publicTransport", "its.ssp.cam.publicTransport", FT_UINT16, BASE_DEC, NULL, 0x4000, NULL, HFILL }},
    { &hf_camssp_specialTransport, { "specialTransport", "its.ssp.cam.specialTransport", FT_UINT16, BASE_DEC, NULL, 0x2000, NULL, HFILL }},
    { &hf_camssp_dangerousGoods, { "dangerousGoods", "its.ssp.cam.dangerousGoods", FT_UINT16, BASE_DEC, NULL, 0x1000, NULL, HFILL }},
    { &hf_camssp_roadwork, { "roadwork", "its.ssp.cam.roadwork", FT_UINT16, BASE_DEC, NULL, 0x0800, NULL, HFILL }},
    { &hf_camssp_rescue, { "rescue", "its.ssp.cam.rescue", FT_UINT16, BASE_DEC, NULL, 0x0400, NULL, HFILL }},
    { &hf_camssp_emergency, { "emergency", "its.ssp.cam.emergency", FT_UINT16, BASE_DEC, NULL, 0x0200, NULL, HFILL }},
    { &hf_camssp_safetyCar, { "safetyCar", "its.ssp.cam.safetyCar", FT_UINT16, BASE_DEC, NULL, 0x0100, NULL, HFILL }},
    { &hf_camssp_closedLanes, { "closedLanes", "its.ssp.cam.closedLanes", FT_UINT16, BASE_DEC, NULL, 0x0080, NULL, HFILL }},
    { &hf_camssp_requestForRightOfWay, { "requestForRightOfWay", "its.ssp.cam.requestForRightOfWay", FT_UINT16, BASE_DEC, NULL, 0x0040, NULL, HFILL }},
    { &hf_camssp_requestForFreeCrossingAtATrafficLight, { "reqFreeCrossTrafLight", "its.ssp.cam.requestForFreeCrossingAtATrafficLight", FT_UINT16, BASE_DEC, NULL, 0x0020, NULL, HFILL }},
    { &hf_camssp_noPassing, { "noPassing", "its.ssp.cam.noPassing", FT_UINT16, BASE_DEC, NULL, 0x0010, NULL, HFILL }},
    { &hf_camssp_noPassingForTrucks, { "noPassingForTrucks", "its.ssp.cam.noPassingForTrucks", FT_UINT16, BASE_DEC, NULL, 0x0008, NULL, HFILL }},
    { &hf_camssp_speedLimit, { "speedLimit", "its.ssp.cam.speedLimit", FT_UINT16, BASE_DEC, NULL, 0x0004, NULL, HFILL }},
    { &hf_camssp_reserved, { "reserved", "its.ssp.cam.reserved", FT_UINT16, BASE_DEC, NULL, 0x0003, NULL, HFILL }},
    };

    static gint *ett[] = {
        &ett_its,
        &ett_denmssp_flags,
        &ett_camssp_flags,

/*--- Included file: packet-its-ettarr.c ---*/
#line 1 "./asn1/its/packet-its-ettarr.c"

/* --- Module ITS-Container --- --- ---                                       */

    &ett_its_ItsPduHeader,
    &ett_its_ReferencePosition,
    &ett_its_DeltaReferencePosition,
    &ett_its_Altitude,
    &ett_its_PosConfidenceEllipse,
    &ett_its_PathPoint,
    &ett_its_PtActivation,
    &ett_its_AccelerationControl,
    &ett_its_CauseCode,
    &ett_its_Curvature,
    &ett_its_Heading,
    &ett_its_ClosedLanes,
    &ett_its_Speed,
    &ett_its_LongitudinalAcceleration,
    &ett_its_LateralAcceleration,
    &ett_its_VerticalAcceleration,
    &ett_its_ExteriorLights,
    &ett_its_DangerousGoodsExtended,
    &ett_its_SpecialTransportType,
    &ett_its_LightBarSirenInUse,
    &ett_its_PositionOfOccupants,
    &ett_its_VehicleIdentification,
    &ett_its_EnergyStorageType,
    &ett_its_VehicleLength,
    &ett_its_PathHistory,
    &ett_its_EmergencyPriority,
    &ett_its_SteeringWheelAngle,
    &ett_its_YawRate,
    &ett_its_ActionID,
    &ett_its_ItineraryPath,
    &ett_its_ProtectedCommunicationZone,
    &ett_its_Traces,
    &ett_its_PositionOfPillars,
    &ett_its_RestrictedTypes,
    &ett_its_EventHistory,
    &ett_its_EventPoint,
    &ett_its_ProtectedCommunicationZonesRSU,
    &ett_its_CenDsrcTollingZone,
    &ett_its_DigitalMap,

/* --- Module AVIAEINumberingAndDataStructures --- --- ---                    */


/* --- Module ElectronicRegistrationIdentificationVehicleDataModule --- --- --- */

    &ett_erivdm_EuVehicleCategoryCode,

/* --- Module CITSapplMgmtIDs --- --- ---                                     */

    &ett_csmid_VarLengthNumber,
    &ett_csmid_Ext1,
    &ett_csmid_Ext2,

/* --- Module EfcDsrcApplication --- --- ---                                  */

    &ett_dsrc_app_AxleWeightLimits,
    &ett_dsrc_app_DieselEmissionValues,
    &ett_dsrc_app_T_particulate,
    &ett_dsrc_app_EnvironmentalCharacteristics,
    &ett_dsrc_app_ExhaustEmissionValues,
    &ett_dsrc_app_PassengerCapacity,
    &ett_dsrc_app_Provider,
    &ett_dsrc_app_SoundLevel,
    &ett_dsrc_app_VehicleDimensions,
    &ett_dsrc_app_VehicleWeightLimits,

/* --- Module IVI --- --- ---                                                 */

    &ett_ivi_IviStructure,
    &ett_ivi_SEQUENCE_SIZE_1_8__OF_IviContainer,
    &ett_ivi_IviContainer,
    &ett_ivi_IVIManagementContainer,
    &ett_ivi_SEQUENCE_SIZE_1_8_OF_IviIdentificationNumber,
    &ett_ivi_GeographicLocationContainer,
    &ett_ivi_SEQUENCE_SIZE_1_16__OF_GlcPart,
    &ett_ivi_GlcPart,
    &ett_ivi_GeneralIviContainer,
    &ett_ivi_GicPart,
    &ett_ivi_T_GicPartDetectionZoneIds,
    &ett_ivi_T_GicPartRelevanceZoneIds,
    &ett_ivi_T_GicPartDriverAwarenessZoneIds,
    &ett_ivi_SEQUENCE_SIZE_1_8__OF_LanePosition,
    &ett_ivi_SEQUENCE_SIZE_1_8__OF_CompleteVehicleCharacteristics,
    &ett_ivi_SEQUENCE_SIZE_1_4__OF_RSCode,
    &ett_ivi_T_GicPartExtraText,
    &ett_ivi_RoadConfigurationContainer,
    &ett_ivi_RccPart,
    &ett_ivi_SEQUENCE_SIZE_1_8__OF_Zid,
    &ett_ivi_SEQUENCE_SIZE_1_16__OF_LaneInformation,
    &ett_ivi_TextContainer,
    &ett_ivi_TcPart,
    &ett_ivi_T_TcPartDetectionZoneIds,
    &ett_ivi_T_TcPartRelevanceZoneIds,
    &ett_ivi_T_TcPartDriverAwarenessZoneIds,
    &ett_ivi_T_TcPartText,
    &ett_ivi_LayoutContainer,
    &ett_ivi_SEQUENCE_SIZE_1_4__OF_LayoutComponent,
    &ett_ivi_AbsolutePosition,
    &ett_ivi_AbsolutePositionWAltitude,
    &ett_ivi_AnyCatalogue,
    &ett_ivi_CompleteVehicleCharacteristics,
    &ett_ivi_SEQUENCE_SIZE_1_3_OF_TrailerCharacteristics,
    &ett_ivi_ComputedSegment,
    &ett_ivi_DeltaPosition,
    &ett_ivi_Distance,
    &ett_ivi_DistanceOrDuration,
    &ett_ivi_ISO14823Attributes,
    &ett_ivi_ISO14823Attributes_item,
    &ett_ivi_ISO14823Code,
    &ett_ivi_T_icPictogramCode,
    &ett_ivi_T_serviceCategoryCode,
    &ett_ivi_T_pictogramCategoryCode,
    &ett_ivi_LaneInformation,
    &ett_ivi_LayoutComponent,
    &ett_ivi_LoadType,
    &ett_ivi_PolygonalLine,
    &ett_ivi_SEQUENCE_SIZE_1_32__OF_DeltaPosition,
    &ett_ivi_SEQUENCE_SIZE_1_32__OF_DeltaReferencePosition,
    &ett_ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePosition,
    &ett_ivi_SEQUENCE_SIZE_1_8__OF_AbsolutePositionWAltitude,
    &ett_ivi_RSCode,
    &ett_ivi_T_code,
    &ett_ivi_Segment,
    &ett_ivi_Text,
    &ett_ivi_TractorCharacteristics,
    &ett_ivi_T_TractorCharactEqualTo,
    &ett_ivi_T_TractorCharactNotEqualTo,
    &ett_ivi_SEQUENCE_SIZE_1_4__OF_VehicleCharacteristicsRanges,
    &ett_ivi_TrailerCharacteristics,
    &ett_ivi_T_TrailerCharactEqualTo,
    &ett_ivi_T_TrailerCharactNotEqualTo,
    &ett_ivi_VcCode,
    &ett_ivi_SEQUENCE_SIZE_1_8__OF_DTM,
    &ett_ivi_VehicleCharacteristicsFixValues,
    &ett_ivi_VehicleCharacteristicsRanges,
    &ett_ivi_T_limits,
    &ett_ivi_Weight,
    &ett_ivi_Zone,
    &ett_ivi_DTM,
    &ett_ivi_T_year,
    &ett_ivi_T_month_day,
    &ett_ivi_T_hourMinutes,
    &ett_ivi_MonthDay,
    &ett_ivi_PMD,
    &ett_ivi_HoursMinutes,
    &ett_ivi_DayOfWeek,
    &ett_ivi_VED,
    &ett_ivi_SPE,
    &ett_ivi_DDD,
    &ett_ivi_SEQUENCE_SIZE_1_8__OF_DDD_IO,
    &ett_ivi_DDD_IO,
    &ett_ivi_SEQUENCE_SIZE_1_4__OF_DestinationPlace,
    &ett_ivi_SEQUENCE_SIZE_1_4__OF_DestinationRoad,
    &ett_ivi_DestinationPlace,
    &ett_ivi_DestinationRoad,

/* --- Module DSRC --- --- ---                                                */

    &ett_dsrc_RegionalExtension,
    &ett_dsrc_SPAT,
    &ett_dsrc_T_SPATRegional,
    &ett_dsrc_SignalRequestMessage,
    &ett_dsrc_T_SRMRegional,
    &ett_dsrc_SignalStatusMessage,
    &ett_dsrc_T_SSMRegional,
    &ett_dsrc_MapData,
    &ett_dsrc_T_MAPRegional,
    &ett_dsrc_AdvisorySpeed,
    &ett_dsrc_T_AdvisorySpeedRegional,
    &ett_dsrc_AdvisorySpeedList,
    &ett_dsrc_ComputedLane,
    &ett_dsrc_T_offsetXaxis,
    &ett_dsrc_T_offsetYaxis,
    &ett_dsrc_T_ComputedLaneRegional,
    &ett_dsrc_ConnectsToList,
    &ett_dsrc_ConnectingLane,
    &ett_dsrc_Connection,
    &ett_dsrc_ConnectionManeuverAssist,
    &ett_dsrc_T_ConnectionManeuverAssistRegional,
    &ett_dsrc_DataParameters,
    &ett_dsrc_EnabledLaneList,
    &ett_dsrc_GenericLane,
    &ett_dsrc_T_GenericLaneRegional,
    &ett_dsrc_IntersectionAccessPoint,
    &ett_dsrc_IntersectionGeometry,
    &ett_dsrc_T_IntersectionGeometryRegional,
    &ett_dsrc_IntersectionGeometryList,
    &ett_dsrc_IntersectionReferenceID,
    &ett_dsrc_IntersectionState,
    &ett_dsrc_T_IntersectionStateRegional,
    &ett_dsrc_IntersectionStateList,
    &ett_dsrc_LaneAttributes,
    &ett_dsrc_LaneDataAttribute,
    &ett_dsrc_T_LaneDataAttributeRegional,
    &ett_dsrc_LaneDataAttributeList,
    &ett_dsrc_LaneList,
    &ett_dsrc_LaneSharing,
    &ett_dsrc_LaneTypeAttributes,
    &ett_dsrc_ManeuverAssistList,
    &ett_dsrc_MovementEventList,
    &ett_dsrc_MovementEvent,
    &ett_dsrc_T_MovementEventRegional,
    &ett_dsrc_MovementList,
    &ett_dsrc_MovementState,
    &ett_dsrc_T_MovementStateRegional,
    &ett_dsrc_NodeAttributeSetXY,
    &ett_dsrc_T_NodeAttributeSetXYRegional,
    &ett_dsrc_NodeAttributeXYList,
    &ett_dsrc_Node_LLmD_64b,
    &ett_dsrc_Node_XY_20b,
    &ett_dsrc_Node_XY_22b,
    &ett_dsrc_Node_XY_24b,
    &ett_dsrc_Node_XY_26b,
    &ett_dsrc_Node_XY_28b,
    &ett_dsrc_Node_XY_32b,
    &ett_dsrc_NodeListXY,
    &ett_dsrc_NodeOffsetPointXY,
    &ett_dsrc_NodeSetXY,
    &ett_dsrc_NodeXY,
    &ett_dsrc_OverlayLaneList,
    &ett_dsrc_Position3D,
    &ett_dsrc_T_Position3DRegional,
    &ett_dsrc_PreemptPriorityList,
    &ett_dsrc_RegulatorySpeedLimit,
    &ett_dsrc_RequestorDescription,
    &ett_dsrc_T_RequestorDescriptionRegional,
    &ett_dsrc_RequestorPositionVector,
    &ett_dsrc_RequestorType,
    &ett_dsrc_RestrictionClassAssignment,
    &ett_dsrc_RestrictionClassList,
    &ett_dsrc_RestrictionUserTypeList,
    &ett_dsrc_RestrictionUserType,
    &ett_dsrc_T_RestrictionUserTypeRegional,
    &ett_dsrc_RoadLaneSetList,
    &ett_dsrc_RoadSegmentReferenceID,
    &ett_dsrc_RoadSegment,
    &ett_dsrc_T_RoadSegmentRegional,
    &ett_dsrc_RoadSegmentList,
    &ett_dsrc_SegmentAttributeXYList,
    &ett_dsrc_SignalControlZone,
    &ett_dsrc_SignalRequesterInfo,
    &ett_dsrc_SignalRequestList,
    &ett_dsrc_SignalRequestPackage,
    &ett_dsrc_T_SignalRequestPackageRegional,
    &ett_dsrc_SignalRequest,
    &ett_dsrc_T_SignalRequestRegional,
    &ett_dsrc_SignalStatusList,
    &ett_dsrc_SignalStatusPackageList,
    &ett_dsrc_SignalStatusPackage,
    &ett_dsrc_T_SignalStatusPackageRegional,
    &ett_dsrc_SignalStatus,
    &ett_dsrc_T_SignalStatusRegional,
    &ett_dsrc_SpeedLimitList,
    &ett_dsrc_TimeChangeDetails,
    &ett_dsrc_TransmissionAndSpeed,
    &ett_dsrc_VehicleID,
    &ett_dsrc_AllowedManeuvers,
    &ett_dsrc_IntersectionStatusObject,
    &ett_dsrc_LaneAttributes_Barrier,
    &ett_dsrc_LaneAttributes_Bike,
    &ett_dsrc_LaneAttributes_Crosswalk,
    &ett_dsrc_LaneAttributes_Parking,
    &ett_dsrc_LaneAttributes_Sidewalk,
    &ett_dsrc_LaneAttributes_Striping,
    &ett_dsrc_LaneAttributes_TrackedVehicle,
    &ett_dsrc_LaneAttributes_Vehicle,
    &ett_dsrc_LaneDirection,
    &ett_dsrc_TransitVehicleStatus,

/* --- Module AddGrpC --- --- ---                                             */

    &ett_AddGrpC_ConnectionManeuverAssist_addGrpC,
    &ett_AddGrpC_ConnectionTrajectory_addGrpC,
    &ett_AddGrpC_Control_addGrpC,
    &ett_AddGrpC_IntersectionState_addGrpC,
    &ett_AddGrpC_MapData_addGrpC,
    &ett_AddGrpC_Position3D_addGrpC,
    &ett_AddGrpC_PrioritizationResponseList,
    &ett_AddGrpC_PrioritizationResponse,
    &ett_AddGrpC_RestrictionUserType_addGrpC,
    &ett_AddGrpC_SignalHeadLocationList,
    &ett_AddGrpC_SignalHeadLocation,
    &ett_AddGrpC_SignalStatusPackage_addGrpC,
    &ett_AddGrpC_ItsStationPositionList,
    &ett_AddGrpC_ItsStationPosition,

/* --- Module REGION --- --- ---                                              */


/* --- Module SPATEM-PDU-Descriptions --- --- ---                             */


/* --- Module MAPEM-PDU-Descriptions --- --- ---                              */


/* --- Module IVIM-PDU-Descriptions --- --- ---                               */


/* --- Module SREM-PDU-Descriptions --- --- ---                               */


/* --- Module SSEM-PDU-Descriptions --- --- ---                               */


/* --- Module CAM-PDU-Descriptions --- --- ---                                */

    &ett_cam_CoopAwareness,
    &ett_cam_CamParameters,
    &ett_cam_HighFrequencyContainer,
    &ett_cam_LowFrequencyContainer,
    &ett_cam_SpecialVehicleContainer,
    &ett_cam_BasicContainer,
    &ett_cam_BasicVehicleContainerHighFrequency,
    &ett_cam_BasicVehicleContainerLowFrequency,
    &ett_cam_PublicTransportContainer,
    &ett_cam_SpecialTransportContainer,
    &ett_cam_DangerousGoodsContainer,
    &ett_cam_RoadWorksContainerBasic,
    &ett_cam_RescueContainer,
    &ett_cam_EmergencyContainer,
    &ett_cam_SafetyCarContainer,
    &ett_cam_RSUContainerHighFrequency,

/* --- Module DENM-PDU-Descriptions --- --- ---                               */

    &ett_denm_DecentralizedEnvironmentalNotificationMessage,
    &ett_denm_ManagementContainer,
    &ett_denm_SituationContainer,
    &ett_denm_LocationContainer,
    &ett_denm_ImpactReductionContainer,
    &ett_denm_RoadWorksContainerExtended,
    &ett_denm_StationaryVehicleContainer,
    &ett_denm_AlacarteContainer,
    &ett_denm_ReferenceDenms,

/* --- Module TIS-TPG-Transactions-Descriptions --- --- ---                   */

    &ett_tistpg_TisTpgTransaction,
    &ett_tistpg_TisTpgDRM,
    &ett_tistpg_TisTpgDRM_Management,
    &ett_tistpg_TisTpgDRM_Situation,
    &ett_tistpg_TisTpgDRM_Location,
    &ett_tistpg_TisTpgSNM,
    &ett_tistpg_TisTpgSNM_Management,
    &ett_tistpg_TisTpgTRM,
    &ett_tistpg_TisTpgTRM_Management,
    &ett_tistpg_TisTpgTRM_Situation,
    &ett_tistpg_TisTpgTRM_Location,
    &ett_tistpg_TisTpgTCM,
    &ett_tistpg_TisTpgTCM_Management,
    &ett_tistpg_TisTpgTCM_Situation,
    &ett_tistpg_TisTpgTCM_Location,
    &ett_tistpg_TisTpgVDRM,
    &ett_tistpg_TisTpgVDRM_Management,
    &ett_tistpg_TisTpgVDPM,
    &ett_tistpg_TisTpgVDPM_Management,
    &ett_tistpg_VehicleSpecificData,
    &ett_tistpg_TisTpgEOFM,
    &ett_tistpg_TisTpgEOFM_Management,
    &ett_tistpg_PlacardTable,
    &ett_tistpg_TyreSetVariant,
    &ett_tistpg_PressureVariantsList,
    &ett_tistpg_PressureVariant,
    &ett_tistpg_TyreData,
    &ett_tistpg_T_currentTyrePressure,
    &ett_tistpg_T_tyreSidewallInformation,
    &ett_tistpg_T_currentInsideAirTemperature,
    &ett_tistpg_T_recommendedTyrePressure,
    &ett_tistpg_T_tin,
    &ett_tistpg_T_sensorState,
    &ett_tistpg_AppliedTyrePressure,
    &ett_tistpg_TpgStationData,
    &ett_tistpg_AppliedTyrePressures,
    &ett_tistpg_TpgNotifContainer,
    &ett_tistpg_TpgAutomation,
    &ett_tistpg_TisProfile,

/* --- Module EVCSN-PDU-Descriptions --- --- ---                              */

    &ett_evcsn_EVChargingSpotNotificationPOIMessage,
    &ett_evcsn_ItsPOIHeader,
    &ett_evcsn_ItsEVCSNData,
    &ett_evcsn_SEQUENCE_SIZE_1_256_OF_ItsChargingStationData,
    &ett_evcsn_ItsChargingStationData,
    &ett_evcsn_ItsChargingSpots,
    &ett_evcsn_ItsChargingSpotDataElements,
    &ett_evcsn_ChargingSpotType,
    &ett_evcsn_ParkingPlacesData,
    &ett_evcsn_SpotAvailability,

/* --- Module EV-RechargingSpotReservation-PDU-Descriptions --- --- ---       */

    &ett_evrsr_EV_RSR_MessageBody,
    &ett_evrsr_PreReservationRequestMessage,
    &ett_evrsr_PreReservationResponseMessage,
    &ett_evrsr_ReservationRequestMessage,
    &ett_evrsr_ReservationResponseMessage,
    &ett_evrsr_CancellationRequestMessage,
    &ett_evrsr_CancellationResponseMessage,
    &ett_evrsr_UpdateRequestMessage,
    &ett_evrsr_UpdateResponseMessage,
    &ett_evrsr_Payment_ID,
    &ett_evrsr_RechargingType,
    &ett_evrsr_SupportedPaymentTypes,

/*--- End of included file: packet-its-ettarr.c ---*/
#line 582 "./asn1/its/packet-its-template.c"
    };

    proto_its = proto_register_protocol("Intelligent Transport Systems", "ITS", "its");

    proto_register_field_array(proto_its, hf_its, array_length(hf_its));

    proto_register_subtree_array(ett, array_length(ett));

    register_dissector("its", dissect_its_PDU, proto_its);

    // Register subdissector table
    its_version_subdissector_table = register_dissector_table("its.version", "ITS version", proto_its, FT_UINT8, BASE_DEC);
    its_msgid_subdissector_table = register_dissector_table("its.msg_id", "ITS message id", proto_its, FT_UINT32, BASE_DEC);
    regionid_subdissector_table = register_dissector_table("dsrc.regionid", "DSRC RegionId", proto_its, FT_UINT32, BASE_DEC);

    proto_its_denm = proto_register_protocol_in_name_only("ITS message - DENM", "DENM", "its.message.denm", proto_its, FT_BYTES);
    proto_its_cam = proto_register_protocol_in_name_only("ITS message - CAM", "CAM", "its.message.cam", proto_its, FT_BYTES);
    proto_its_spatem = proto_register_protocol_in_name_only("ITS message - SPATEM", "SPATEM", "its.message.spatem", proto_its, FT_BYTES);
    proto_its_mapem = proto_register_protocol_in_name_only("ITS message - MAPEM", "MAPEM", "its.message.mapem", proto_its, FT_BYTES);
    proto_its_ivim = proto_register_protocol_in_name_only("ITS message - IVIM", "IVIM", "its.message.ivim", proto_its, FT_BYTES);
    proto_its_evrsr = proto_register_protocol_in_name_only("ITS message - EVRSR", "EVRSR", "its.message.evrsr", proto_its, FT_BYTES);
    proto_its_srem = proto_register_protocol_in_name_only("ITS message - SREM", "SREM", "its.message.srem", proto_its, FT_BYTES);
    proto_its_ssem = proto_register_protocol_in_name_only("ITS message - SSEM", "SSEM", "its.message.ssem", proto_its, FT_BYTES);
    proto_its_evcsn = proto_register_protocol_in_name_only("ITS message - EVCSN", "EVCSN", "its.message.evcsn", proto_its, FT_BYTES);
    proto_its_tistpg = proto_register_protocol_in_name_only("ITS message - TISTPG", "TISTPG", "its.message.tistpg", proto_its, FT_BYTES);

    proto_addgrpc = proto_register_protocol_in_name_only("DSRC Addition Grp C (EU)", "ADDGRPC", "dsrc.addgrpc", proto_its, FT_BYTES);

    // Decode as
    static build_valid_func its_da_build_value[1] = {its_msgid_value};
    static decode_as_value_t its_da_values = {its_msgid_prompt, 1, its_da_build_value};
    static decode_as_t its_da = {"its", "ITS msg id", "its.msg_id", 1, 0, &its_da_values, NULL, NULL,
                                    decode_as_default_populate_list, decode_as_default_reset, decode_as_default_change, NULL};

    register_decode_as(&its_da);
}

#define BTP_SUBDISS_SZ 2
#define BTP_PORTS_SZ   10
void proto_reg_handoff_its(void)
{
    const char *subdissector[BTP_SUBDISS_SZ] = { "btpa.port", "btpb.port" };
    const guint16 ports[BTP_PORTS_SZ] = { ITS_WKP_DEN, ITS_WKP_CA, ITS_WKP_EVCSN, ITS_WKP_CHARGING, ITS_WKP_IVI, ITS_WKP_TPG, ITS_WKP_TLC_SSEM, ITS_WKP_TLC_SREM, ITS_WKP_RLT, ITS_WKP_TLM };
    int sdIdx, pIdx;
    dissector_handle_t its_handle_;

    // Register well known ports to btp subdissector table (BTP A and B)
    its_handle_ = create_dissector_handle(dissect_its_PDU, proto_its);
    for (sdIdx=0; sdIdx < BTP_SUBDISS_SZ; sdIdx++) {
        for (pIdx=0; pIdx < BTP_PORTS_SZ; pIdx++) {
            dissector_add_uint(subdissector[sdIdx], ports[pIdx], its_handle_);
        }
    }

    dissector_add_uint("its.msg_id", ITS_DENM,              create_dissector_handle( dissect_denm_DecentralizedEnvironmentalNotificationMessage_PDU, proto_its_denm ));
    dissector_add_uint("its.msg_id", ITS_CAM,               create_dissector_handle( dissect_cam_CoopAwareness_PDU, proto_its_cam ));
    dissector_add_uint("its.msg_id", ITS_SPATEM,            create_dissector_handle( dissect_dsrc_SPAT_PDU, proto_its_spatem ));
    dissector_add_uint("its.msg_id", ITS_MAPEM,             create_dissector_handle( dissect_dsrc_MapData_PDU, proto_its_mapem ));
    dissector_add_uint("its.msg_id", ITS_IVIM,              create_dissector_handle( dissect_ivi_IviStructure_PDU, proto_its_ivim ));
    dissector_add_uint("its.msg_id", ITS_EV_RSR,            create_dissector_handle( dissect_evrsr_EV_RSR_MessageBody_PDU, proto_its_evrsr ));
    dissector_add_uint("its.msg_id", ITS_SREM,              create_dissector_handle( dissect_dsrc_SignalRequestMessage_PDU, proto_its_srem ));
    dissector_add_uint("its.msg_id", ITS_SSEM,              create_dissector_handle( dissect_dsrc_SignalStatusMessage_PDU, proto_its_ssem ));
    dissector_add_uint("its.msg_id", ITS_EVCSN,             create_dissector_handle( dissect_evcsn_EVChargingSpotNotificationPOIMessage_PDU, proto_its_evcsn ));
    dissector_add_uint("its.msg_id", ITS_TISTPGTRANSACTION, create_dissector_handle( dissect_tistpg_TisTpgTransaction_PDU, proto_its_tistpg ));

    /* Missing definitions: ITS_POI, ITS_SAEM, ITS_RTCMEM */

    dissector_add_uint("dsrc.regionid", (addGrpC<<16)+Reg_ConnectionManeuverAssist, create_dissector_handle(dissect_AddGrpC_ConnectionManeuverAssist_addGrpC_PDU, proto_addgrpc ));
    dissector_add_uint("dsrc.regionid", (addGrpC<<16)+Reg_GenericLane, create_dissector_handle(dissect_AddGrpC_ConnectionTrajectory_addGrpC_PDU, proto_addgrpc ));
    dissector_add_uint("dsrc.regionid", (addGrpC<<16)+Reg_NodeAttributeSetXY, create_dissector_handle(dissect_AddGrpC_Control_addGrpC_PDU, proto_addgrpc ));
    dissector_add_uint("dsrc.regionid", (addGrpC<<16)+Reg_IntersectionState, create_dissector_handle(dissect_AddGrpC_IntersectionState_addGrpC_PDU, proto_addgrpc ));
    dissector_add_uint("dsrc.regionid", (addGrpC<<16)+Reg_MapData, create_dissector_handle(dissect_AddGrpC_MapData_addGrpC_PDU, proto_addgrpc ));
    dissector_add_uint("dsrc.regionid", (addGrpC<<16)+Reg_Position3D, create_dissector_handle(dissect_AddGrpC_Position3D_addGrpC_PDU, proto_addgrpc ));
    dissector_add_uint("dsrc.regionid", (addGrpC<<16)+Reg_RestrictionUserType, create_dissector_handle(dissect_AddGrpC_RestrictionUserType_addGrpC_PDU, proto_addgrpc ));
    dissector_add_uint("dsrc.regionid", (addGrpC<<16)+Reg_SignalStatusPackage, create_dissector_handle(dissect_AddGrpC_SignalStatusPackage_addGrpC_PDU, proto_addgrpc ));

    dissector_add_uint("ieee1609dot2.ssp", psid_den_basic_services, create_dissector_handle(dissect_denmssp_pdu, proto_its_denm));
    dissector_add_uint("ieee1609dot2.ssp", psid_ca_basic_services,  create_dissector_handle(dissect_camssp_pdu, proto_its_cam));
    dissector_add_uint("geonw.ssp", psid_den_basic_services, create_dissector_handle(dissect_denmssp_pdu, proto_its_denm));
    dissector_add_uint("geonw.ssp", psid_ca_basic_services,  create_dissector_handle(dissect_camssp_pdu, proto_its_cam));

    its_tap = register_tap("its");
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
