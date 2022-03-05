// This is the main DLL file.

#include "Stdafx.h"
#include "MetaKitWrapper.h"

using namespace MetaKitWrapper;
using namespace System::Runtime::InteropServices;

MetaKit::~MetaKit()
{
    if(_Manager != NULL)
        _Manager->CloseDB();
}

bool MetaKit::OpenDB(String^ filename)
{
    auto p = Marshal::StringToHGlobalAnsi(filename);

    _FileName = (char *)p.ToPointer();
        
    _Manager = new MetaKitManager();

    _Manager->OpenDB(_FileName);

    Marshal::FreeHGlobal(p);

    return _Manager->IsOpen();
}


bool MetaKit::CloseDB()
{
    if(_Manager->IsOpen())
    {
        _Manager->CloseDB();
    }
    return !_Manager->IsOpen();
}


int MetaKit::GetDataItem(int ViewNum, int RowNum, int ColNum, String^& FieldType,
            String^& StrRet, int& IntRet, double& DblRet)
{
    char* field = NULL;
    CString* strRet = NULL;
    long intRet = 0;
    double doubleRet = 0;

    int ret = _Manager->GetDataItem(ViewNum, RowNum, ColNum, field, strRet, &intRet, &doubleRet);

    FieldType = gcnew String(field);
    if (strRet != NULL)
        StrRet = gcnew String(*strRet);
    IntRet = intRet;
    DblRet = doubleRet;

    return ret;
}

array<Byte>^ MetaKit::FindBinaryByKey(int viewNum, uint64_t key, int keyColNum, int colNum) {
    array<Byte> ^output = nullptr;
    int ret = _Manager->FindBinaryByKey(viewNum, key, keyColNum, colNum, output);

    return output;
}

KeyValuePair<uint64_t, array<Byte>^>
MetaKit::GetKeyBinaryValuePair(int viewNum, int rowNum, int keyColNum, int valColNum) {
    uint64_t keyOutput = 0;
    array<Byte> ^valOutput = nullptr;
    int ret = _Manager->GetKeyBinaryValuePair(viewNum, rowNum, keyColNum, valColNum, keyOutput, valOutput);

    return KeyValuePair<uint64_t, array<Byte>^>(keyOutput, valOutput);
}

String^ MetaKit::GetString(int viewNum, int rowNum, int columnNum)
{
    CString str = "";
    int ret = _Manager->GetString(viewNum, (int)rowNum, columnNum, str);

    return gcnew String(str.GetBuffer(str.GetLength()));
}

int MetaKit::GetInt(int viewNum, int rowNum, int columnNum)
{
    int i = -1;
    int ret = _Manager->GetInt(viewNum, rowNum, columnNum, &i);

    return i;
}

long MetaKit::GetLong(int viewNum, int rowNum, int columnNum)
{
    long l = -1;
    int ret = _Manager->GetLong(viewNum, rowNum, columnNum, &l);

    return l;
}

float MetaKit::GetFloat(int viewNum, int rowNum, int columnNum)
{
    float f = -1;
    int ret = _Manager->GetFloat(viewNum, rowNum, columnNum, &f);

    return f;
}

double MetaKit::GetDouble(int viewNum, int rowNum, int columnNum)
{
    double d = -1;
    int ret = _Manager->GetDouble(viewNum, rowNum, columnNum, &d);

    return d;
}



int MetaKit::OpenView(String^ viewName)
{
    auto p = Marshal::StringToHGlobalAnsi(viewName);

    char* viewname = (char *)p.ToPointer();

    auto viewId = _Manager->OpenAView(viewname);

    Marshal::FreeHGlobal(p);

    return viewId;
}

int MetaKit::GetRowCount(int viewNum)
{
    return _Manager->GetRowCount(viewNum);
}
//-------------------------------------------
//-------------------------------------------

MetaKitManager::MetaKitManager()
{
    try
    {
        _Views = 0;
        _DbOpen = false;
    }
    catch(int e)
    {

    }
}

bool MetaKitManager::IsOpen()
{
    return _DbOpen;
}


bool MetaKitManager::CloseDB()
{
    try
    {
        if(_DbOpen)
        {
            DELETEANDNULL(_Database);
            DELETEANDNULL(_FileName);

            for(int i=0; i < _Views; i++)
            {
                m_ptrActiveTableName[i] = NULL;
                m_ptrActiveTableStructure[i] = NULL;
            }
        }

        _DbOpen = false;
    }
    catch (int e)
    {
        
    }

    return !_DbOpen;
}



bool MetaKitManager::OpenDB(char* filename)
{
    try
    {
        if(_DbOpen)
        {
            CloseDB();
        }

        _FileName = new char[strlen(filename) +1];
        strcpy(_FileName, filename);

        //for (int i = 0; i < VGMAXOPEN; i++)
        //{
            //m_ptrActiveTableName[i] = NULL;
            //m_ptrActiveTableStructure[i] = NULL;
        //}

         _Database = new c4_Storage(_FileName, true);
         _DbOpen = true;
         _Views = 0;
    }
    catch(int e)
    {
        _DbOpen = false;
    }

    return _DbOpen;
}

int MetaKitManager::GetDataItem(const int ViewNum, const long RowNum, const int ColNum, char* FieldType,
                CString* StrRet, long* LongOrIntRet, double* DoubleOrFloatRet)
{
    int value;
    value = 0;  // set return value
    
    CString cs;       // temp string to hold contents
    c4_RowRef r = m_ActiveViewHandle[ViewNum][RowNum];
    c4_Property p = m_ActiveViewHandle[ViewNum].NthProperty(ColNum);
    
    // handle the data in the MK4 storage types
    *FieldType = p.Type();
    switch (*FieldType)
    {
        case 'I':
            *LongOrIntRet = (int)((c4_IntProp&) p)(r);
            break;
        
        case 'L':
            *LongOrIntRet = (long)((c4_LongProp&) p)(r);
            break;
        
        case 'F':
            *DoubleOrFloatRet = (float)((c4_FloatProp&) p)(r);
            break;
        
        case 'D':
            *DoubleOrFloatRet = (double)((c4_DoubleProp&) p)(r);
            break;
        
        case 'S':
            *StrRet = (const char*)((c4_StringProp&) p)(r);
            break;
        
            /*    do not handle memos or byte fields initially, may need to include
            case 'M': // backward compatibility
            case 'B':
            (p(r)).GetData(data);
            sprintf(buff, " (%db)", data.Size());
            break;
            */
        
        default:
            value = 1;  
    }

    return value;
}

int MetaKitManager::FindBinaryByKey(const int viewNum, const uint64_t key, const int keyColNum, const int colNum,
    array<Byte>^ %output) {

    auto &view = m_ActiveViewHandle[viewNum];
    auto _keyProp = view.NthProperty(keyColNum);
    auto _valProp = view.NthProperty(colNum);

    if (_valProp.Type() != 'B' || _keyProp.Type() != 'L')
        return 1;

    auto &keyProp = (c4_LongProp&)_keyProp;
    auto &valProp = (c4_BytesProp&)_valProp;

    c4_Row findRow;
    keyProp(findRow) = key;

    auto foundRowIdx = view.Find(findRow, 0);
    if (foundRowIdx < 0)
        return 1;

    auto bytes = valProp(view[foundRowIdx]).Access(0);
    output = gcnew array<Byte>(bytes.Size());
    pin_ptr<Byte> p = &output[0];
    Byte *np = p;
    memcpy(np, bytes.Contents(), bytes.Size());
    return 0;
}

int MetaKitManager::GetKeyBinaryValuePair(const int viewNum, const int rowNum, const int keyColNum, const int valColNum,
    uint64_t &keyOutput, array<Byte>^ %valOutput) {

    auto &view = m_ActiveViewHandle[viewNum];
    auto _keyProp = view.NthProperty(keyColNum);
    auto _valProp = view.NthProperty(valColNum);

    if (_valProp.Type() != 'B' || _keyProp.Type() != 'L')
        return 1;

    auto &keyProp = (c4_LongProp&)_keyProp;
    auto &valProp = (c4_BytesProp&)_valProp;

    auto rowRef = view[rowNum];
    keyOutput = (uint64_t)keyProp(rowRef);
    auto bytes = valProp(rowRef).Access(0);
    valOutput = gcnew array<Byte>(bytes.Size());
    pin_ptr<Byte> p = &valOutput[0];
    Byte *np = p;
    memcpy(np, bytes.Contents(), bytes.Size());

    return 0;
}

int MetaKitManager::GetString(const int ViewNum, const long RowNum, const int ColNum, CString& RetVal)
{
    int value;
    value = 0; // set the status return value
    
    // these are possible return values from call to get mk4 data
    char FieldType; // the storage type in Mk4-tells which ret value is filled
    // only one of these return variables will have a value
    CString StrRet; // returned if type is string
    long LongOrIntRet; // returned if type is long
    double DoubleOrFloatRet; // returned if type is double
    
    value = 0;    // clear error return value
    
    // get the typed value from the mk4 table-stored type in FieldType
    GetDataItem(ViewNum, RowNum, ColNum, &FieldType, &StrRet, 
        &LongOrIntRet, &DoubleOrFloatRet);
    // convert the value to the requested type
    switch (FieldType)
    {
        case 'I':
        case 'L':
            RetVal.Format(TEXT("%ld"), LongOrIntRet);
            break;
        
        case 'F':
            RetVal.Format(TEXT("%g"), DoubleOrFloatRet);
            break;
        
        case 'D':
            RetVal.Format(TEXT("%.15g"), DoubleOrFloatRet);
            break;
        
        case 'S':
            RetVal = StrRet;
            break;
        
            /*    do not handle memos or byte fields initially, may need to include
            case 'M': // backward compatibility
            case 'B':
            (p(r)).GetData(data);
            sprintf(buff, " (%db)", data.Size());
            break;
            */
        
        default:
            // dont know what to do so return error
            value = 1;
    }
    
    
    return value;
}//@CODE_202



int MetaKitManager::GetInt(const int ViewNum, const long RowNum, const int ColNum, int* RetVal)
{
    int value;
    
    // these are possible return values from call to get mk4 data
    char FieldType; // the storage type in Mk4-tells which ret value is filled
    // only one of these return variables will have a value
    CString StrRet; // returned if type is string
    long LongOrIntRet; // returned if type is long
    double DoubleOrFloatRet; // returned if type is double
    
    value = 0;    // clear error return value
    // get the typed value from the mk4 table-stored type in FieldType
    GetDataItem(ViewNum, RowNum, ColNum, &FieldType, &StrRet, &LongOrIntRet, &DoubleOrFloatRet);
    // convert the value to the requested type
    switch (FieldType)
    {
        case 'I':
        case 'L':
            *RetVal = LongOrIntRet;
            break;
        
        case 'F':
        case 'D':
            *RetVal = (int)(DoubleOrFloatRet + 0.5);
            break;
        
        case 'S':
            *RetVal = _ttoi(StrRet);
            break;
        
            /*    do not handle memos or byte fields initially, may need to include
            case 'M': // backward compatibility
            case 'B':
            (p(r)).GetData(data);
            sprintf(buff, " (%db)", data.Size());
            break;
            */
        
        default:
            // dont know what to do so return error
            *RetVal = 0;
            value = 1;
    }
    
    
    return value;
}//@CODE_197



int MetaKitManager::GetFloat(const int ViewNum, const long RowNum, const int ColNum, float* RetVal)
{
    int value;
    // these are possible return values from call to get mk4 data
    char FieldType; // the storage type in Mk4-tells which ret value is filled
    // only one of these return variables will have a value
    CString StrRet; // returned if type is string
    long LongOrIntRet; // returned if type is long
    double DoubleOrFloatRet; // returned if type is double
    // TCHAR* stopchar; // character that stops conversion
    
    value = 0;    // clear error return value
    // get the typed value from the mk4 table-stored type in FieldType
    GetDataItem(ViewNum, RowNum, ColNum, &FieldType, &StrRet, 
        &LongOrIntRet, &DoubleOrFloatRet);
    // convert the value to the requested type
    switch (FieldType)
    {
        case 'I':
        case 'L':
            *RetVal = (float)LongOrIntRet;
            break;
        
        case 'F':
        case 'D':
            *RetVal = (float)DoubleOrFloatRet;
            break;
        
        case 'S':
            *RetVal = (float)(_tcstod(StrRet, NULL));
            break;
        
            /*    do not handle memos or byte fields initially, may need to include
            case 'M': // backward compatibility
            case 'B':
            (p(r)).GetData(data);
            sprintf(buff, " (%db)", data.Size());
            break;
            */
        
        default:
            // dont know what to do so return error
            *RetVal = 0;
            value = 1;
    }
    
    
    return value;
}//@CODE_207



int MetaKitManager::GetDouble(const int ViewNum, const long RowNum, const int ColNum, double* RetVal)
{
    int value;
    
    // these are possible return values from call to get mk4 data
    char FieldType; // the storage type in Mk4-tells which ret value is filled
    // only one of these return variables will have a value
    CString StrRet; // returned if type is string
    long LongOrIntRet; // returned if type is long
    double DoubleOrFloatRet; // returned if type is double
    // TCHAR* stopchar; // character that stops conversion
    
    value = 0;    // clear error return value
    
    // get the typed value from the mk4 table-stored type in FieldType
    GetDataItem(ViewNum, RowNum, ColNum, &FieldType, &StrRet, 
        &LongOrIntRet, &DoubleOrFloatRet);
    // convert the value to the requested type
    switch (FieldType)
    {
        case 'I':
        case 'L':
            *RetVal = LongOrIntRet;
            break;
        
        case 'F':
        case 'D':
            *RetVal = DoubleOrFloatRet;
            break;
        
        case 'S':
            *RetVal = _tcstod(StrRet, NULL);
            break;
        
            /*    do not handle memos or byte fields initially, may need to include
            case 'M': // backward compatibility
            case 'B':
            (p(r)).GetData(data);
            sprintf(buff, " (%db)", data.Size());
            break;
            */
        
        default:
            // dont know what to do so return error
            *RetVal = 0;
            value = 1;
    }
    
    
    return value;
}

int MetaKitManager::GetLong(const int ViewNum, const long RowNum, const int ColNum, long* RetVal)
{
    int value;
    
    // these are possible return values from call to get mk4 data
    char FieldType; // the storage type in Mk4-tells which ret value is filled
    // only one of these return variables will have a value
    CString StrRet; // returned if type is string
    long LongOrIntRet; // returned if type is long
    double DoubleOrFloatRet; // returned if type is double
    
    value = 0;    // clear error return value
    // get the typed value from the mk4 table-stored type in FieldType
    GetDataItem(ViewNum, RowNum, ColNum, &FieldType, &StrRet, 
        &LongOrIntRet, &DoubleOrFloatRet);
    // convert the value to the requested type
    switch (FieldType)
    {
        case 'I':
        case 'L':
            *RetVal = LongOrIntRet;
            break;
        
        case 'F':
        case 'D':
            *RetVal = (long)(DoubleOrFloatRet + 0.5);
            break;
        
        case 'S':
            *RetVal = _ttol(StrRet);
            break;
        
            /*    do not handle memos or byte fields initially, may need to include
            case 'M': // backward compatibility
            case 'B':
            (p(r)).GetData(data);
            sprintf(buff, " (%db)", data.Size());
            break;
            */
        
        default:
            // dont know what to do so return error
            *RetVal = 0;
            value = 1;
    }
    
    
    return value;
}



int MetaKitManager::StringToChar(const CString StringToConvert, char** ptrCharArray)
{
    int value = 0;
    
    value = StringToConvert.GetLength();
    *ptrCharArray = new char[value + 1];
    value = wcstombs(*ptrCharArray, StringToConvert, value + 1);
    
    return value;
}


int MetaKitManager::OpenAView(CString sAViewName)
{
    int value;
    value = -1;      // initialize return value
    
    // convert the string into a char
    char* ptrAViewName = NULL;
    StringToChar(sAViewName, &ptrAViewName);
    
    CString sTemp;
    char* cTemp = NULL; // temporary character pointer
    

    for (int i = 0; i < VGMAXOPEN; i++)
    {
        if (m_ptrActiveTableName[i] != NULL)
        {
            if (sAViewName.Compare(CString(m_ptrActiveTableName[i])) == 0)
            {
                return i;
            }
        }
    }
    
    try
    {

        // *** *** *** ***
        // Find an open slot in the array to stick this
        for (int i = 0; i < VGMAXOPEN; i++)
        {
            if (m_ptrActiveTableName[i] == NULL)
            {
                // found a free slot, so open the view, and gather all info about this view
                
                // save the table name
                m_ptrActiveTableName[i] = (char*) new char[(strlen(ptrAViewName) +1)];
                strcpy(m_ptrActiveTableName[i], ptrAViewName);
                
                // retrieve and save the table structure
                m_ptrActiveTableStructure[i] = (char*) new char[(strlen(_Database->Description(ptrAViewName)) + 1)];
                strcpy(m_ptrActiveTableStructure[i], _Database->Description(ptrAViewName));
                
                // open the db view by building the GetAs structure string
                if (strlen(m_ptrActiveTableStructure[i]) > 0)
                {
                    // make up a char array with the table structure to include name and columns
                    sTemp = m_ptrActiveTableName[i];
                    sTemp += TEXT("[");
                    sTemp += m_ptrActiveTableStructure[i];
                    sTemp += TEXT("]");
                    // get rid of old part
                    DELETEANDNULLALL(m_ptrActiveTableStructure[i]);
                    // save the complete structure
                    m_ptrActiveTableStructure[i] = (char*) new char[sTemp.GetLength() + 1];
                    StringToChar(sTemp, &cTemp);//
                    strcpy(m_ptrActiveTableStructure[i], cTemp);// sTemp
                    DELETEANDNULLALL(cTemp);
                }
                
                // open view as instantiated with View member rather than with the GetAs
                m_ActiveViewHandle[i] = _Database->View(ptrAViewName);
                
                // then set up for retrun
                value = i;  // set up return value of the index being used
                _Views++;
                
                break;          // since we completed processing, just break out of for loop
            } // end of process when empty slot found
        }   // end of for loop
    }
    catch(int e)
    {

    }
    finally
    {
        DELETEANDNULLALL(ptrAViewName);    // clean up allocation
    }

    return value;
}


int MetaKitManager::GetRowCount(int ViewNum)
{
    int ret = -1;

    if(ViewNum <= _Views)
    {
        ret = m_ActiveViewHandle[ViewNum].GetSize();
    }

    return ret;
}