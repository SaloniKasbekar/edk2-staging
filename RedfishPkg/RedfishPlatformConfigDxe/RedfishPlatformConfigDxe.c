/** @file

  The implementation of EDKII Redfidh Platform Config Protocol.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishPlatformConfigDxe.h"
#include "RedfishPlatformConfigImpl.h"

REDFISH_PLATFORM_CONFIG_PRIVATE *mRedfishPlatformConfigPrivate = NULL;

/**
  Compare two value in HII statement format.

  @param[in]  Value1        Firt value to compare.
  @param[in]  Value2        Second value to be compared.

  @retval UINTN         0 is retuned when two values are equal.
                        1 is returned when first value is greater than second value.
                        -1 is returned when second value is greater than first value.

**/
UINTN
CompareHiiStatementValue (
  IN HII_STATEMENT_VALUE  *Value1,
  IN HII_STATEMENT_VALUE  *Value2
  )
{
  INTN Result;
  UINT64 Data1;
  UINT64 Data2;

  if (Value1 == NULL || Value2 == NULL) {
    return 0xFF;
  }

  switch (Value1->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      Data1 = Value1->Value.u8;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      Data1 = Value1->Value.u16;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      Data1 = Value1->Value.u32;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_64:
      Data1 = Value1->Value.u64;
      break;
    case EFI_IFR_TYPE_BOOLEAN:
      Data1 = (Value1->Value.b ? 1 : 0);
      break;
    default:
      return 0xFF;
  }

  switch (Value2->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      Data2 = Value2->Value.u8;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      Data2 = Value2->Value.u16;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      Data2 = Value2->Value.u32;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_64:
      Data2 = Value2->Value.u64;
      break;
    case EFI_IFR_TYPE_BOOLEAN:
      Data2 = (Value2->Value.b ? 1 : 0);
      break;
    default:
      return 0xFF;
  }

  Result = (Data1 == Data2 ? 0 : (Data1 > Data2 ? 1 : -1));

  return Result;
}

/**
  Convert HII value to the string in HII one-of opcode.

  @param[in]  Statement     Statement private instance

  @retval EFI_STRING_ID     The string ID in HII database.
                            0 is returned when something goes wrong.

**/
EFI_STRING_ID
HiiValueToOneOfOptionStringId (
  IN REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE *Statement
  )
{
  LIST_ENTRY            *Link;
  HII_QUESTION_OPTION   *Option;

  if (Statement->HiiStatement->Operand != EFI_IFR_ONE_OF_OP) {
    return 0;
  }

  if (IsListEmpty (&Statement->HiiStatement->OptionListHead)) {
    return 0;
  }

  Link = GetFirstNode (&Statement->HiiStatement->OptionListHead);
  while (!IsNull (&Statement->HiiStatement->OptionListHead, Link)) {
    Option = HII_QUESTION_OPTION_FROM_LINK (Link);

    if (CompareHiiStatementValue (&Statement->HiiStatement->Value, &Option->Value) == 0) {
      return Option->Text;
    }

    Link = GetNextNode (&Statement->HiiStatement->OptionListHead, Link);
  }

  return 0;
}

/**
  Convert HII string to the value in HII one-of opcode.

  @param[in]  Statement     Statement private instance
  @param[in]  Schema        Schema string
  @param[in]  HiiString     Input string
  @param[out] Value         Value returned

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
HiiStringToOneOfOptionValue (
  IN  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE *Statement,
  IN  CHAR8                           *Schema,
  IN  EFI_STRING                      HiiString,
  OUT HII_STATEMENT_VALUE             *Value
  )
{
  LIST_ENTRY            *Link;
  HII_QUESTION_OPTION   *Option;
  EFI_STRING            TmpString;
  BOOLEAN               Found;

  if (Statement == NULL || IS_EMPTY_STRING (HiiString) || Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Statement->HiiStatement->Operand != EFI_IFR_ONE_OF_OP) {
    return EFI_UNSUPPORTED;
  }

  if (IsListEmpty (&Statement->HiiStatement->OptionListHead)) {
    return EFI_NOT_FOUND;
  }

  Found = FALSE;
  Link = GetFirstNode (&Statement->HiiStatement->OptionListHead);
  while (!IsNull (&Statement->HiiStatement->OptionListHead, Link)) {
    Option = HII_QUESTION_OPTION_FROM_LINK (Link);

    TmpString = HiiGetRedfishString (Statement->ParentForm->ParentFormset->HiiHandle, Schema, Option->Text);
    if (TmpString != NULL) {
      if (StrCmp (TmpString, HiiString) == 0) {
        CopyMem (Value, &Option->Value, sizeof (HII_STATEMENT_VALUE));
        Found = TRUE;
      }
      FreePool (TmpString);
    }

    if (Found) {
      return EFI_SUCCESS;
    }

    Link = GetNextNode (&Statement->HiiStatement->OptionListHead, Link);
  }

  return EFI_NOT_FOUND;
}

/**
  Convert HII value to numeric value in Redfish format.

  @param[in]  Value         Value to be converted.
  @param[out] RedfishValue  Value in Redfish format.

  @retval EFI_SUCCESS       Redfish value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
HiiValueToRedfishNumeric (
  IN  HII_STATEMENT_VALUE  *Value,
  OUT EDKII_REDFISH_VALUE  *RedfishValue
  )
{
  if (Value == NULL || RedfishValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Value->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      RedfishValue->Type = REDFISH_VALUE_TYPE_INTEGER;
      RedfishValue->Value.Integer = (INT64)Value->Value.u8;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      RedfishValue->Type = REDFISH_VALUE_TYPE_INTEGER;
      RedfishValue->Value.Integer = (INT64)Value->Value.u16;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      RedfishValue->Type = REDFISH_VALUE_TYPE_INTEGER;
      RedfishValue->Value.Integer = (INT64)Value->Value.u32;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_64:
      RedfishValue->Type = REDFISH_VALUE_TYPE_INTEGER;
      RedfishValue->Value.Integer = (INT64)Value->Value.u64;
      break;
    case EFI_IFR_TYPE_BOOLEAN:
      RedfishValue->Type = REDFISH_VALUE_TYPE_BOOLEAN;
      RedfishValue->Value.Boolean = Value->Value.b;
      break;
    default:
      RedfishValue->Type = REDFISH_VALUE_TYPE_UNKNOWN;
      break;
  }

  return EFI_SUCCESS;
}

/**
  Convert numeric value in Redfish format to HII value.

  @param[in]   RedfishValue  Value in Redfish format to be converted.
  @param[out]  Value         HII value returned.

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
RedfishNumericToHiiValue (
  IN  EDKII_REDFISH_VALUE  *RedfishValue,
  OUT HII_STATEMENT_VALUE  *Value
  )
{
  if (Value == NULL || RedfishValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (RedfishValue->Type) {
    case REDFISH_VALUE_TYPE_INTEGER:
      Value->Type = EFI_IFR_TYPE_NUM_SIZE_64;
      Value->Value.u64 = (UINT64)RedfishValue->Value.Integer;
      break;
    case REDFISH_VALUE_TYPE_BOOLEAN:
      Value->Type = EFI_IFR_TYPE_BOOLEAN;
      Value->Value.b = RedfishValue->Value.Boolean;
      break;
    default:
      Value->Type = EFI_IFR_TYPE_UNDEFINED;
      break;
  }

  return EFI_SUCCESS;
}

/**
  Return the full Redfish schema string from the given Schema and Version.

  Returned schema string is: Schema + '.' + Version

  @param[in]  Schema      Schema string
  @param[in]  Version     Schema version string

  @retval CHAR8 *         Schema string. NULL when errors occur.

**/
CHAR8 *
GetFullSchemaString (
  IN CHAR8   *Schema,
  IN CHAR8  *Version
  )
{
  UINTN Size;
  CHAR8 *FullName;

  if (IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Version)) {
    return NULL;
  }

  Size = AsciiStrSize(CONFIGURE_LANGUAGE_PREFIX) + AsciiStrSize (Schema) + AsciiStrSize (Version);

  FullName = AllocatePool (Size);
  if (FullName == NULL) {
    DEBUG ((DEBUG_ERROR, "%a, out-of-resource\n", __FUNCTION__));
    return NULL;
  }

  AsciiSPrint (FullName, Size, "%a%a.%a", CONFIGURE_LANGUAGE_PREFIX, Schema, Version);

  return FullName;
}

/**
  Common implementation to get statement private instance.

  @param[in]   RedfishPlatformConfigPrivate   Private instance.
  @param[in]   Schema                         Redfish schema string.
  @param[in]   ConfigureLang                  Configure language that refers to this statement.
  @param[out]  Statement                      Statement instance

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
RedfishPlatformConfigGetStatementCommon (
  IN     REDFISH_PLATFORM_CONFIG_PRIVATE            *RedfishPlatformConfigPrivate,
  IN     CHAR8                                      *Schema,
  IN     EFI_STRING                                 ConfigureLang,
  OUT    REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  **Statement
  )
{
  EFI_STATUS                      Status;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE *TargetStatement;

  if (RedfishPlatformConfigPrivate == NULL || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (ConfigureLang) || Statement == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Statement = NULL;

  Status = ProcessPendingList (&RedfishPlatformConfigPrivate->FormsetList, &RedfishPlatformConfigPrivate->PendingList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, ProcessPendingList failure: %r\n", __FUNCTION__, Status));
    return Status;
  }

  TargetStatement = GetStatementPrivateByConfigureLang (&RedfishPlatformConfigPrivate->FormsetList, Schema, ConfigureLang);
  if (TargetStatement == NULL) {
    DEBUG ((DEBUG_ERROR, "%a, No match HII statement is found by the given %s in schema %a\n", __FUNCTION__, ConfigureLang, Schema));
    return EFI_NOT_FOUND;
  }

  //
  // Find current HII question value.
  //
  Status = GetQuestionValue (
             TargetStatement->ParentForm->ParentFormset->HiiFormSet,
             TargetStatement->ParentForm->HiiForm,
             TargetStatement->HiiStatement,
             GetSetValueWithHiiDriver
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, failed to get question current value: %r\n", __FUNCTION__, Status));
    return Status;
  }


  if (TargetStatement->HiiStatement->Value.Type == EFI_IFR_TYPE_UNDEFINED) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Return Value.
  //
  *Statement = TargetStatement;

  return EFI_SUCCESS;
}

/**
  Get Redfish value with the given Schema and Configure Language.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[out]  Value               The returned value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigProtocolGetValue (
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL *This,
  IN     CHAR8                                  *Schema,
  IN     CHAR8                                  *Version,
  IN     EFI_STRING                             ConfigureLang,
  OUT    EDKII_REDFISH_VALUE                    *Value
  )
{
  EFI_STATUS                                Status;
  REDFISH_PLATFORM_CONFIG_PRIVATE           *RedfishPlatformConfigPrivate;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE *TargetStatement;
  EFI_STRING_ID                             StringId;
  CHAR8                                     *FullSchema;
  EFI_STRING                                HiiString;
  UINTN                                     Size;

  if (This == NULL || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Version) || IS_EMPTY_STRING (ConfigureLang) || Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  RedfishPlatformConfigPrivate = REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);
  Value->Type = REDFISH_VALUE_TYPE_UNKNOWN;
  FullSchema = NULL;
  HiiString = NULL;

  FullSchema = GetFullSchemaString (Schema, Version);
  if (FullSchema == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = RedfishPlatformConfigGetStatementCommon (RedfishPlatformConfigPrivate, FullSchema, ConfigureLang, &TargetStatement);
  if (EFI_ERROR (Status)) {
    goto RELEASE_RESOURCE;
  }

  switch (TargetStatement->HiiStatement->Operand) {
    case EFI_IFR_ONE_OF_OP:
      StringId = HiiValueToOneOfOptionStringId (TargetStatement);
      if (StringId == 0) {
        ASSERT (FALSE);
        Status = EFI_DEVICE_ERROR;
        goto RELEASE_RESOURCE;
      }

      HiiString = HiiGetRedfishString (TargetStatement->ParentForm->ParentFormset->HiiHandle, FullSchema, StringId);
      if (HiiString == NULL) {
        DEBUG ((DEBUG_ERROR, "%a, Can not find string ID: 0x%x with %a\n", __FUNCTION__, StringId, FullSchema));
        Status = EFI_NOT_FOUND;
        goto RELEASE_RESOURCE;
      }

      Size = StrLen (HiiString) + 1;
      Value->Value.Buffer = AllocatePool (Size);
      if (Value->Value.Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto RELEASE_RESOURCE;
      }

      UnicodeStrToAsciiStrS (HiiString, Value->Value.Buffer, Size);
      Value->Type = REDFISH_VALUE_TYPE_STRING;

      break;
    case EFI_IFR_STRING_OP:
      if (TargetStatement->HiiStatement->Value.Type != EFI_IFR_TYPE_STRING) {
        ASSERT (FALSE);
        Status = EFI_DEVICE_ERROR;
        goto RELEASE_RESOURCE;
      }

      Value->Type = REDFISH_VALUE_TYPE_STRING;
      Value->Value.Buffer = AllocateCopyPool (StrSize ((CHAR16 *)TargetStatement->HiiStatement->Value.Buffer), TargetStatement->HiiStatement->Value.Buffer);
      if (Value->Value.Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto RELEASE_RESOURCE;
      }
      break;
    case EFI_IFR_CHECKBOX_OP:
    case EFI_IFR_NUMERIC_OP:
      Status = HiiValueToRedfishNumeric (&TargetStatement->HiiStatement->Value, Value);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a, failed to convert HII value to Redfish value: %r\n", __FUNCTION__, Status));
        goto RELEASE_RESOURCE;
      }
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a, catch unsupported type: 0x%x! Please contact with author if we need to support this type.\n", __FUNCTION__, TargetStatement->HiiStatement->Operand));
      ASSERT (FALSE);
      Status = EFI_UNSUPPORTED;
      goto RELEASE_RESOURCE;
  }

RELEASE_RESOURCE:

  if (FullSchema != NULL) {
    FreePool (FullSchema);
  }

  if (HiiString != NULL) {
    FreePool (HiiString);
  }

  return Status;
}

/**
  Function to save question value into HII database.

  @param[in]   HiiFormset       HII form-set instance
  @param[in]   HiiForm          HII form instance
  @param[in]   HiiStatement     HII statement that keeps new value.
  @param[in]   Value            New value to applyu.

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
RedfishPlatformConfigSaveQuestionValue (
  IN  HII_FORMSET         *HiiFormset,
  IN  HII_FORM            *HiiForm,
  IN  HII_STATEMENT       *HiiStatement,
  IN  HII_STATEMENT_VALUE *Value
  )
{
  EFI_STATUS  Status;

  if (HiiFormset == NULL || HiiForm == NULL || HiiStatement == NULL || Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SetQuestionValue (
             HiiFormset,
             HiiForm,
             HiiStatement,
             Value
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, failed to set question value: %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = SubmitForm (HiiFormset, HiiForm);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, failed to submit form: %r\n", __FUNCTION__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Common implementation to set statement private instance.

  @param[in]   RedfishPlatformConfigPrivate   Private instance.
  @param[in]   Schema                         Redfish schema string.
  @param[in]   ConfigureLang                  Configure language that refers to this statement.
  @param[in]   Statement                      Statement instance

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
RedfishPlatformConfigSetStatementCommon (
  IN     REDFISH_PLATFORM_CONFIG_PRIVATE  *RedfishPlatformConfigPrivate,
  IN     CHAR8                            *Schema,
  IN     EFI_STRING                       ConfigureLang,
  IN     HII_STATEMENT_VALUE              *StatementValue
  )
{
  EFI_STATUS                                Status;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE *TargetStatement;
  EFI_STRING                                TempBuffer;

  if (RedfishPlatformConfigPrivate == NULL || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (ConfigureLang) || StatementValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TempBuffer = NULL;

  Status = ProcessPendingList (&RedfishPlatformConfigPrivate->FormsetList, &RedfishPlatformConfigPrivate->PendingList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, ProcessPendingList failure: %r\n", __FUNCTION__, Status));
    return Status;
  }

  TargetStatement = GetStatementPrivateByConfigureLang (&RedfishPlatformConfigPrivate->FormsetList, Schema, ConfigureLang);
  if (TargetStatement == NULL) {
    DEBUG ((DEBUG_ERROR, "%a, No match HII statement is found by the given %s in schema %a\n", __FUNCTION__, ConfigureLang, Schema));
    return EFI_NOT_FOUND;
  }

  if (StatementValue->Type != TargetStatement->HiiStatement->Value.Type) {
    //
    // We treat one-of type as string in Redfish. But one-of statement is not
    // in string format from HII point of view. Do a patch here.
    //
    if (TargetStatement->HiiStatement->Operand == EFI_IFR_ONE_OF_OP && StatementValue->Type == EFI_IFR_TYPE_STRING) {
      TempBuffer = AllocatePool (StatementValue->BufferLen * sizeof (CHAR16));
      if (TempBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      AsciiStrToUnicodeStrS (StatementValue->Buffer, TempBuffer, StatementValue->BufferLen);
      FreePool (StatementValue->Buffer);
      StatementValue->Buffer = NULL;
      StatementValue->BufferLen = 0;

      Status = HiiStringToOneOfOptionValue (TargetStatement, Schema, TempBuffer, StatementValue);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a, failed to find option value by the given %s\n", __FUNCTION__, TempBuffer));
        FreePool (TempBuffer);
        return EFI_NOT_FOUND;
      }

      FreePool (TempBuffer);
    } else if (TargetStatement->HiiStatement->Operand == EFI_IFR_NUMERIC_OP && StatementValue->Type == EFI_IFR_TYPE_NUM_SIZE_64) {
      //
      // Redfish only has numeric value type and it does not care about the value size.
      // Do a patch here so we have proper value size applied.
      //
      StatementValue->Type = TargetStatement->HiiStatement->Value.Type;
    } else {
      DEBUG ((DEBUG_ERROR, "%a, catch value type mismatch! input type: 0x%x but target value type: 0x%x\n", __FUNCTION__, StatementValue->Type, TargetStatement->HiiStatement->Value.Type));
      ASSERT (FALSE);
    }
  }

  Status = RedfishPlatformConfigSaveQuestionValue (
             TargetStatement->ParentForm->ParentFormset->HiiFormSet,
             TargetStatement->ParentForm->HiiForm,
             TargetStatement->HiiStatement,
             StatementValue
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, failed to save question value: %r\n", __FUNCTION__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Set Redfish value with the given Schema and Configure Language.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[in]   Value               The value to set.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigProtocolSetValue (
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL *This,
  IN     CHAR8                                  *Schema,
  IN     CHAR8                                  *Version,
  IN     EFI_STRING                             ConfigureLang,
  IN     EDKII_REDFISH_VALUE                    Value
  )
{
  EFI_STATUS                                Status;
  REDFISH_PLATFORM_CONFIG_PRIVATE           *RedfishPlatformConfigPrivate;
  CHAR8                                     *FullSchema;
  HII_STATEMENT_VALUE                       NewValue;

  if (This == NULL || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Version) || IS_EMPTY_STRING (ConfigureLang)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Value.Type == REDFISH_VALUE_TYPE_UNKNOWN || Value.Type >= REDFISH_VALUE_TYPE_MAX) {
    return EFI_INVALID_PARAMETER;
  }

  RedfishPlatformConfigPrivate = REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);
  FullSchema = NULL;

  FullSchema = GetFullSchemaString (Schema, Version);
  if (FullSchema == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (&NewValue, sizeof (HII_STATEMENT_VALUE));

  switch (Value.Type) {
    case REDFISH_VALUE_TYPE_INTEGER:
    case REDFISH_VALUE_TYPE_BOOLEAN:
      Status = RedfishNumericToHiiValue (&Value, &NewValue);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a, failed to convert Redfish value to Hii value: %r\n", __FUNCTION__, Status));
        goto RELEASE_RESOURCE;
      }
      break;
    case REDFISH_VALUE_TYPE_STRING:
      NewValue.Type = EFI_IFR_TYPE_STRING;
      NewValue.BufferLen = (UINT16)AsciiStrSize (Value.Value.Buffer);
      NewValue.Buffer = AllocateCopyPool (NewValue.BufferLen, Value.Value.Buffer);
      if (NewValue.Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto RELEASE_RESOURCE;
      }
      break;
    default:
      ASSERT (FALSE);
      break;
  }

  Status = RedfishPlatformConfigSetStatementCommon (RedfishPlatformConfigPrivate, FullSchema, ConfigureLang, &NewValue);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, failed to set value to statement: %r\n", __FUNCTION__, Status));
  }

RELEASE_RESOURCE:

  if (FullSchema != NULL) {
    FreePool (FullSchema);
  }

  return Status;
}

/**
  Get the list of Configure Language from platform configuration by the given Schema and Pattern.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   Pattern             The target Configure Language pattern.
  @param[out]  ConfigureLangList         The list of Configure Language.
  @param[out]  Count               The number of Configure Language in ConfigureLangList.

  @retval EFI_SUCCESS              ConfigureLangList is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigProtocolGetConfigureLang (
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL *This,
  IN     CHAR8                                  *Schema,
  IN     CHAR8                                  *Version,
  IN     EFI_STRING                             Pattern,
  OUT    EFI_STRING                             **ConfigureLangList,
  OUT    UINTN                                  *Count
  )
{
  REDFISH_PLATFORM_CONFIG_PRIVATE                 *RedfishPlatformConfigPrivate;
  EFI_STATUS                                      Status;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_LIST  StatementList;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_REF   *StatementRef;
  LIST_ENTRY                                      *NextLink;
  EFI_STRING                                      TmpString;
  EFI_STRING                                      *TmpConfigureLangList;
  UINTN                                           Index;
  CHAR8                                           *FullSchema;

  if (This == NULL || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Version) || Count == NULL || ConfigureLangList == NULL || IS_EMPTY_STRING (Pattern)) {
    return EFI_INVALID_PARAMETER;
  }

  *Count = 0;
  *ConfigureLangList = NULL;
  FullSchema = NULL;
  RedfishPlatformConfigPrivate = REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);

  Status = ProcessPendingList (&RedfishPlatformConfigPrivate->FormsetList, &RedfishPlatformConfigPrivate->PendingList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, ProcessPendingList failure: %r\n", __FUNCTION__, Status));
    return Status;
  }

  FullSchema = GetFullSchemaString (Schema, Version);
  if (FullSchema == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GetStatementPrivateByConfigureLangRegex (
             RedfishPlatformConfigPrivate->RegularExpressionProtocol,
             &RedfishPlatformConfigPrivate->FormsetList,
             FullSchema,
             Pattern,
             &StatementList
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, GetStatementPrivateByConfigureLangRegex failure: %r\n", __FUNCTION__, Status));
    goto RELEASE_RESOURCE;
  }

  if (!IsListEmpty (&StatementList.StatementList)) {

    TmpConfigureLangList = AllocateZeroPool (sizeof (CHAR16 *) * StatementList.Count);
    if (TmpConfigureLangList == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto RELEASE_RESOURCE;
    }

    Index = 0;
    NextLink = GetFirstNode (&StatementList.StatementList);
    while (!IsNull (&StatementList.StatementList, NextLink)) {
      StatementRef = REDFISH_PLATFORM_CONFIG_STATEMENT_REF_FROM_LINK (NextLink);
      NextLink = GetNextNode (&StatementList.StatementList, NextLink);

      ASSERT (StatementRef->Statement->Description != 0);
      if (StatementRef->Statement->Description != 0) {
        TmpString = HiiGetRedfishString (StatementRef->Statement->ParentForm->ParentFormset->HiiHandle, FullSchema, StatementRef->Statement->Description);
        ASSERT (TmpString != NULL);
        if (TmpString != NULL) {
          TmpConfigureLangList[Index] = AllocateCopyPool (StrSize (TmpString), TmpString);
          ASSERT (TmpConfigureLangList[Index] != NULL);
          FreePool (TmpString);
          ++Index;
        }
      }
    }
  }

  *Count = StatementList.Count;
  *ConfigureLangList = TmpConfigureLangList;

RELEASE_RESOURCE:

  if (FullSchema != NULL) {
    FreePool (FullSchema);
  }

  ReleaseStatementList (&StatementList);

  return Status;
}


/**
  Get the list of supported Redfish schema from paltform configuration on give HII handle.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   HiiHandle           The target handle to search. If handle is NULL,
                                   this function return all schema from HII database.
  @param[out]  SupportedSchema     The supported schema list which is separated by ';'.
                                   The SupportedSchema is allocated by the callee. It's caller's
                                   responsibility to free this buffer using FreePool().

  @retval EFI_SUCCESS              Schema is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigProtocolGetSupportedSchema (
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL   *This,
  IN     EFI_HII_HANDLE                           HiiHandle,       OPTIONAL
  OUT    CHAR8                                    **SupportedSchema
  )
{
  REDFISH_PLATFORM_CONFIG_PRIVATE           *RedfishPlatformConfigPrivate;
  EFI_STATUS                                Status;
  LIST_ENTRY                                *HiiFormsetLink;
  LIST_ENTRY                                *HiiFormsetNextLink;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *HiiFormsetPrivate;
  UINTN                                     Index;
  UINTN                                     StringSize;
  CHAR8                                     *StringBuffer;
  UINTN                                     StringIndex;

  if (This == NULL || SupportedSchema == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *SupportedSchema = NULL;

  RedfishPlatformConfigPrivate = REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);

  Status = ProcessPendingList (&RedfishPlatformConfigPrivate->FormsetList, &RedfishPlatformConfigPrivate->PendingList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, ProcessPendingList failure: %r\n", __FUNCTION__, Status));
    return Status;
  }

  if (IsListEmpty (&RedfishPlatformConfigPrivate->FormsetList)) {
    return EFI_NOT_FOUND;
  }

  //
  // Calculate for string buffer size.
  //
  StringSize = 0;
  HiiFormsetLink = GetFirstNode (&RedfishPlatformConfigPrivate->FormsetList);
  while (!IsNull (&RedfishPlatformConfigPrivate->FormsetList, HiiFormsetLink)) {
    HiiFormsetNextLink = GetNextNode (&RedfishPlatformConfigPrivate->FormsetList, HiiFormsetLink);
    HiiFormsetPrivate = REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK (HiiFormsetLink);

    if (HiiHandle != NULL && HiiHandle != HiiFormsetPrivate->HiiHandle) {
      HiiFormsetLink = HiiFormsetNextLink;
      continue;
    }

    if (HiiFormsetPrivate->SupportedSchema.Count > 0) {
      for (Index = 0; Index < HiiFormsetPrivate->SupportedSchema.Count; Index++) {
        StringSize += AsciiStrSize (HiiFormsetPrivate->SupportedSchema.SchemaList[Index]);
      }
    }

    HiiFormsetLink = HiiFormsetNextLink;
  }

  if (StringSize == 0) {
    return EFI_NOT_FOUND;
  }

  StringBuffer = AllocatePool (StringSize);
  if (StringBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StringIndex = 0;
  HiiFormsetLink = GetFirstNode (&RedfishPlatformConfigPrivate->FormsetList);
  while (!IsNull (&RedfishPlatformConfigPrivate->FormsetList, HiiFormsetLink)) {
    HiiFormsetNextLink = GetNextNode (&RedfishPlatformConfigPrivate->FormsetList, HiiFormsetLink);
    HiiFormsetPrivate = REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK (HiiFormsetLink);

    if (HiiHandle != NULL && HiiHandle != HiiFormsetPrivate->HiiHandle) {
      HiiFormsetLink = HiiFormsetNextLink;
      continue;
    }

    if (HiiFormsetPrivate->SupportedSchema.Count > 0) {
      for (Index = 0; Index < HiiFormsetPrivate->SupportedSchema.Count; Index++) {
        AsciiStrCpyS (&StringBuffer[StringIndex], (StringSize - StringIndex), HiiFormsetPrivate->SupportedSchema.SchemaList[Index]);
        StringIndex += AsciiStrLen (HiiFormsetPrivate->SupportedSchema.SchemaList[Index]);
        StringBuffer[StringIndex] = ';';
        ++StringIndex;
      }
    }

    HiiFormsetLink = HiiFormsetNextLink;
  }

  StringBuffer[--StringIndex] = '\0';

  *SupportedSchema = StringBuffer;

  return EFI_SUCCESS;
}

/**
  Functions which are registered to receive notification of
  database events have this prototype. The actual event is encoded
  in NotifyType. The following table describes how PackageType,
  PackageGuid, Handle, and Package are used for each of the
  notification types.

  @param[in] PackageType  Package type of the notification.
  @param[in] PackageGuid  If PackageType is
                          EFI_HII_PACKAGE_TYPE_GUID, then this is
                          the pointer to the GUID from the Guid
                          field of EFI_HII_PACKAGE_GUID_HEADER.
                          Otherwise, it must be NULL.
  @param[in] Package      Points to the package referred to by the
                          notification Handle The handle of the package
                          list which contains the specified package.
  @param[in] Handle       The HII handle.
  @param[in] NotifyType   The type of change concerning the
                          database. See
                          EFI_HII_DATABASE_NOTIFY_TYPE.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigFormUpdateNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  EFI_STATUS  Status;

  if (NotifyType == EFI_HII_DATABASE_NOTIFY_NEW_PACK || NotifyType == EFI_HII_DATABASE_NOTIFY_ADD_PACK) {
    //
    // HII formset on this handle is updated by driver during run-time. The formset needs to be reloaded.
    //
    Status = NotifyFormsetUpdate (Handle, &mRedfishPlatformConfigPrivate->PendingList);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a, failed to notify updated formset of HII handle: 0x%x\n", __FUNCTION__, Handle));
      return Status;
    }
  } else if (NotifyType == EFI_HII_DATABASE_NOTIFY_REMOVE_PACK) {
    //
    // HII resource is removed. The formset is no longer exist.
    //
    Status = NotifyFormsetDeleted (Handle, &mRedfishPlatformConfigPrivate->PendingList);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a, failed to notify deleted formset of HII handle: 0x%x\n", __FUNCTION__, Handle));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  This is a EFI_HII_STRING_PROTOCOL notification event handler.

  Install HII package notification.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
HiiStringProtocolInstalled (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS  Status;

  //
  // Locate HII database protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiStringProtocolGuid,
                  NULL,
                  (VOID **)&mRedfishPlatformConfigPrivate->HiiString
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, locate EFI_HII_STRING_PROTOCOL failure: %r\n", __FUNCTION__, Status));
    return;
  }

  gBS->CloseEvent (Event);
  mRedfishPlatformConfigPrivate->HiiStringNotify.ProtocolEvent = NULL;
}

/**
  This is a EFI_HII_DATABASE_PROTOCOL notification event handler.

  Install HII package notification.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
HiiDatabaseProtocolInstalled (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS  Status;

  //
  // Locate HII database protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&mRedfishPlatformConfigPrivate->HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, locate EFI_HII_DATABASE_PROTOCOL failure: %r\n", __FUNCTION__, Status));
    return;
  }

  //
  // Register package notification when new form package is installed.
  //
  Status = mRedfishPlatformConfigPrivate->HiiDatabase->RegisterPackageNotify (
                                             mRedfishPlatformConfigPrivate->HiiDatabase,
                                             EFI_HII_PACKAGE_FORMS,
                                             NULL,
                                             RedfishPlatformConfigFormUpdateNotify,
                                             EFI_HII_DATABASE_NOTIFY_NEW_PACK,
                                             &mRedfishPlatformConfigPrivate->NotifyHandle
                                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, RegisterPackageNotify for EFI_HII_DATABASE_NOTIFY_NEW_PACK failure: %r\n", __FUNCTION__, Status));
  }

  //
  // Register package notification when new form package is updated.
  //
  Status = mRedfishPlatformConfigPrivate->HiiDatabase->RegisterPackageNotify (
                                             mRedfishPlatformConfigPrivate->HiiDatabase,
                                             EFI_HII_PACKAGE_FORMS,
                                             NULL,
                                             RedfishPlatformConfigFormUpdateNotify,
                                             EFI_HII_DATABASE_NOTIFY_ADD_PACK,
                                             &mRedfishPlatformConfigPrivate->NotifyHandle
                                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, RegisterPackageNotify for EFI_HII_DATABASE_NOTIFY_NEW_PACK failure: %r\n", __FUNCTION__, Status));
  }

#if REDFISH_PLATFORM_CONFIG_DELETE_EXPIRED_FORMSET
  //
  // Register package notification when new form package is removed.
  //
  Status = mRedfishPlatformConfigPrivate->HiiDatabase->RegisterPackageNotify (
                                             mRedfishPlatformConfigPrivate->HiiDatabase,
                                             EFI_HII_PACKAGE_FORMS,
                                             NULL,
                                             RedfishPlatformConfigFormUpdateNotify,
                                             EFI_HII_DATABASE_NOTIFY_REMOVE_PACK,
                                             &mRedfishPlatformConfigPrivate->NotifyHandle
                                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, RegisterPackageNotify for EFI_HII_DATABASE_NOTIFY_NEW_PACK failure: %r\n", __FUNCTION__, Status));
  }
#endif

  gBS->CloseEvent (Event);
  mRedfishPlatformConfigPrivate->HiiDbNotify.ProtocolEvent = NULL;

}

/**
  This is a EFI_REGULAR_EXPRESSION_PROTOCOL notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
RegexProtocolInstalled (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS  Status;

  //
  // Locate regular expression protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiRegularExpressionProtocolGuid,
                  NULL,
                  (VOID **)&mRedfishPlatformConfigPrivate->RegularExpressionProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, locate EFI_REGULAR_EXPRESSION_PROTOCOL failure: %r\n", __FUNCTION__, Status));
    return;
  }

  gBS->CloseEvent (Event);
  mRedfishPlatformConfigPrivate->RegexNotify.ProtocolEvent = NULL;

}

/**
  Unloads an image.

  @param  ImageHandle           Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval EFI_INVALID_PARAMETER ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigDxeUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS Status;

  if (mRedfishPlatformConfigPrivate != NULL) {
    Status = gBS->UninstallProtocolInterface (
                    mRedfishPlatformConfigPrivate->ImageHandle,
                    &gEdkIIRedfishPlatformConfigProtocolGuid,
                    (VOID*)&mRedfishPlatformConfigPrivate->Protocol
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a, can not uninstall gEdkIIRedfishPlatformConfigProtocolGuid: %r\n", __FUNCTION__, Status));
      ASSERT (FALSE);
    }

    //
    // Close events
    //
    if (mRedfishPlatformConfigPrivate->HiiDbNotify.ProtocolEvent != NULL) {
      gBS->CloseEvent (mRedfishPlatformConfigPrivate->HiiDbNotify.ProtocolEvent);
    }
    if (mRedfishPlatformConfigPrivate->HiiStringNotify.ProtocolEvent != NULL) {
      gBS->CloseEvent (mRedfishPlatformConfigPrivate->HiiStringNotify.ProtocolEvent);
    }
    if (mRedfishPlatformConfigPrivate->RegexNotify.ProtocolEvent != NULL) {
      gBS->CloseEvent (mRedfishPlatformConfigPrivate->RegexNotify.ProtocolEvent);
    }

    //
    // Unregister package notification.
    //
    if (mRedfishPlatformConfigPrivate->NotifyHandle != NULL) {
      mRedfishPlatformConfigPrivate->HiiDatabase->UnregisterPackageNotify (
                                          mRedfishPlatformConfigPrivate->HiiDatabase,
                                          mRedfishPlatformConfigPrivate->NotifyHandle
                                          );
    }

    ReleaseFormsetList (&mRedfishPlatformConfigPrivate->FormsetList);
    FreePool (mRedfishPlatformConfigPrivate);
    mRedfishPlatformConfigPrivate = NULL;
  }

  return EFI_SUCCESS;
}


/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mRedfishPlatformConfigPrivate = (REDFISH_PLATFORM_CONFIG_PRIVATE *)AllocateZeroPool (sizeof (REDFISH_PLATFORM_CONFIG_PRIVATE));
  if (mRedfishPlatformConfigPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a, can not allocate pool for REDFISH_PLATFORM_CONFIG_PRIVATE\n", __FUNCTION__));
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Protocol initialization
  //
  mRedfishPlatformConfigPrivate->ImageHandle = ImageHandle;
  mRedfishPlatformConfigPrivate->Protocol.GetValue = RedfishPlatformConfigProtocolGetValue;
  mRedfishPlatformConfigPrivate->Protocol.SetValue = RedfishPlatformConfigProtocolSetValue;
  mRedfishPlatformConfigPrivate->Protocol.GetConfigureLang = RedfishPlatformConfigProtocolGetConfigureLang;
  mRedfishPlatformConfigPrivate->Protocol.GetSupportedSchema = RedfishPlatformConfigProtocolGetSupportedSchema;

  InitializeListHead (&mRedfishPlatformConfigPrivate->FormsetList);
  InitializeListHead (&mRedfishPlatformConfigPrivate->PendingList);

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkIIRedfishPlatformConfigProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID*)&mRedfishPlatformConfigPrivate->Protocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, can not install gEdkIIRedfishPlatformConfigProtocolGuid: %r\n", __FUNCTION__, Status));
    ASSERT (FALSE);
  }

  //
  // Install protocol notification if HII database protocol is installed.
  //
  mRedfishPlatformConfigPrivate->HiiDbNotify.ProtocolEvent = EfiCreateProtocolNotifyEvent (
                                                              &gEfiHiiDatabaseProtocolGuid,
                                                              TPL_CALLBACK,
                                                              HiiDatabaseProtocolInstalled,
                                                              NULL,
                                                              &mRedfishPlatformConfigPrivate->HiiDbNotify.Registration
                                                              );
  if (mRedfishPlatformConfigPrivate->HiiDbNotify.ProtocolEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "%a, failed to create protocol notification for gEfiHiiDatabaseProtocolGuid\n", __FUNCTION__));
    ASSERT (FALSE);
  }

  //
  // Install protocol notification if HII string protocol is installed.
  //
  mRedfishPlatformConfigPrivate->HiiStringNotify.ProtocolEvent = EfiCreateProtocolNotifyEvent (
                                                                  &gEfiHiiStringProtocolGuid,
                                                                  TPL_CALLBACK,
                                                                  HiiStringProtocolInstalled,
                                                                  NULL,
                                                                  &mRedfishPlatformConfigPrivate->HiiStringNotify.Registration
                                                                  );
  if (mRedfishPlatformConfigPrivate->HiiStringNotify.ProtocolEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "%a, failed to create protocol notification for gEfiHiiStringProtocolGuid\n", __FUNCTION__));
    ASSERT (FALSE);
  }

  //
  // Install protocol notification if regular expression protocol is installed.
  //
  mRedfishPlatformConfigPrivate->RegexNotify.ProtocolEvent = EfiCreateProtocolNotifyEvent (
                                                              &gEfiRegularExpressionProtocolGuid,
                                                              TPL_CALLBACK,
                                                              RegexProtocolInstalled,
                                                              NULL,
                                                              &mRedfishPlatformConfigPrivate->RegexNotify.Registration
                                                              );
  if (mRedfishPlatformConfigPrivate->RegexNotify.ProtocolEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "%a, failed to create protocol notification for gEfiRegularExpressionProtocolGuid\n", __FUNCTION__));
    ASSERT (FALSE);
  }

  return EFI_SUCCESS;
}