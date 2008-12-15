/*****************************************************************
|
|   Platinum - Service
|
|   Copyright (c) 2004-2008, Plutinosoft, LLC.
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltService.h"
#include "PltSsdp.h"
#include "PltUPnP.h"
#include "PltDeviceData.h"
#include "PltXmlHelper.h"

NPT_SET_LOCAL_LOGGER("platinum.core.service")

/*----------------------------------------------------------------------
|   PLT_Service::PLT_Service
+---------------------------------------------------------------------*/
PLT_Service::PLT_Service(PLT_DeviceData* device,
                         const char*     type, 
                         const char*     id,
                         const char*     last_change_namespace /* = NULL */) :  
    m_Device(device),
    m_ServiceType(type),
    m_ServiceID(id),
    m_EventTask(NULL),
    m_EventingPaused(false),
    m_LastChangeNamespace(last_change_namespace)
{
}

/*----------------------------------------------------------------------
|   PLT_Service::~PLT_Service
+---------------------------------------------------------------------*/
PLT_Service::~PLT_Service()
{
    Cleanup();
}

/*----------------------------------------------------------------------
 |   PLT_Service::~PLT_Service
 +---------------------------------------------------------------------*/
 void
 PLT_Service::Cleanup()
 {
     m_ActionDescs.Apply(NPT_ObjectDeleter<PLT_ActionDesc>());
     m_StateVars.Apply(NPT_ObjectDeleter<PLT_StateVariable>());
     m_Subscribers.Apply(NPT_ObjectDeleter<PLT_EventSubscriber>());

     m_ActionDescs.Clear();
     m_StateVars.Clear();
     m_Subscribers.Clear();
 }

/*----------------------------------------------------------------------
|   PLT_Service::GetSCPDXML
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::GetSCPDXML(NPT_String& scpd)
{
    NPT_Result res;

    // it is required to have at least 1 state variable
    if (m_StateVars.GetItemCount() == 0) return NPT_FAILURE;

    NPT_XmlElementNode* spec = NULL;
    NPT_XmlElementNode* actionList = NULL;
    NPT_XmlElementNode* top = new NPT_XmlElementNode("scpd");
    NPT_XmlElementNode* serviceStateTable = NULL;
    NPT_CHECK_LABEL_SEVERE(res = top->SetNamespaceUri("", "urn:schemas-upnp-org:service-1-0"), cleanup);

    // add spec version
    spec = new NPT_XmlElementNode("specVersion");
    NPT_CHECK_LABEL_SEVERE(res = top->AddChild(spec), cleanup);
    NPT_CHECK_LABEL_SEVERE(res = PLT_XmlHelper::AddChildText(spec, "major", "1"), cleanup);
    NPT_CHECK_LABEL_SEVERE(res = PLT_XmlHelper::AddChildText(spec, "minor", "0"), cleanup);

    // add actions
    actionList = new NPT_XmlElementNode("actionList");
    NPT_CHECK_LABEL_SEVERE(res = top->AddChild(actionList), cleanup);
    NPT_CHECK_LABEL_SEVERE(res = m_ActionDescs.ApplyUntil(PLT_GetSCPDXMLIterator<PLT_ActionDesc>(actionList), 
        NPT_UntilResultNotEquals(NPT_SUCCESS)), cleanup);

    // add service state table
    serviceStateTable = new NPT_XmlElementNode("serviceStateTable");
    NPT_CHECK_LABEL_SEVERE(res = top->AddChild(serviceStateTable), cleanup);
    NPT_CHECK_LABEL_SEVERE(res = m_StateVars.ApplyUntil(PLT_GetSCPDXMLIterator<PLT_StateVariable>(serviceStateTable), 
        NPT_UntilResultNotEquals(NPT_SUCCESS)), cleanup);

    // serialize node
    NPT_CHECK_LABEL_SEVERE(res = PLT_XmlHelper::Serialize(*top, scpd), cleanup);
    
cleanup:
    delete top;
    return res;
}

/*----------------------------------------------------------------------
|   PLT_Service::ToXML
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::GetDescription(NPT_XmlElementNode* parent, NPT_XmlElementNode** service_out /* = NULL */)
{
    NPT_XmlElementNode* service = new NPT_XmlElementNode("service");
    if (service_out) {
        *service_out = service;
    }
    NPT_CHECK_SEVERE(parent->AddChild(service));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(service, "serviceType", m_ServiceType));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(service, "serviceId", m_ServiceID));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(service, "SCPDURL", GetSCPDURL()));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(service, "controlURL", GetControlURL()));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(service, "eventSubURL", GetEventSubURL()));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::InitURLs
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::InitURLs(const char* service_name, 
                      const char* device_uuid)
{
    m_SCPDURL  = service_name;
    m_SCPDURL += "/" + NPT_String(device_uuid) + NPT_String("/scpd.xml");
    m_ControlURL  = service_name;
    m_ControlURL += "/" + NPT_String(device_uuid) + NPT_String("/control.xml");
    m_EventSubURL  = service_name;
    m_EventSubURL += "/" + NPT_String(device_uuid) + NPT_String("/event.xml");
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::SetSCPDXML
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::SetSCPDXML(const char* scpd)
{
    if (scpd == NULL) return NPT_FAILURE;

    Cleanup();

    NPT_XmlParser parser;
    NPT_XmlNode*  tree = NULL;
    NPT_Result    res;
    NPT_Array<NPT_XmlElementNode*> stateVariables;
    NPT_Array<NPT_XmlElementNode*> actions;
    NPT_XmlElementNode* root;
    NPT_XmlElementNode* actionList;
    NPT_XmlElementNode* stateTable;

    res = parser.Parse(scpd, tree);
    NPT_CHECK_LABEL_FATAL(res, failure);

    // make sure root tag is right
    root = tree->AsElementNode();
    if (!root || NPT_String::Compare(root->GetTag(), "scpd")) {
        goto failure;
    }

    // make sure we have required children presents
    actionList = PLT_XmlHelper::GetChild(root, "actionList");
    stateTable = PLT_XmlHelper::GetChild(root, "serviceStateTable");
    if (!actionList || 
        !stateTable || 
        !actionList->GetChildren().GetItemCount() || 
        !stateTable->GetChildren().GetItemCount()) {
        goto failure;
    }

    // stateVariable table
    if (NPT_FAILED(PLT_XmlHelper::GetChildren(stateTable,
                                              stateVariables, 
                                              "stateVariable"))) {
        goto failure;
    }

    for( int k = 0 ; k < (int)stateVariables.GetItemCount(); k++) {
        NPT_String name, type, send;
        PLT_XmlHelper::GetChildText(stateVariables[k], "name", name);
        PLT_XmlHelper::GetChildText(stateVariables[k], "dataType", type);
        PLT_XmlHelper::GetAttribute(stateVariables[k], "sendEvents", send);
        if (name.GetLength() == 0 || type.GetLength() == 0) {
            goto failure;
        }
        PLT_StateVariable* variable = new PLT_StateVariable(this);
        m_StateVars.Add(variable);

        variable->m_Name = name;
        variable->m_DataType = type;
        variable->m_IsSendingEvents = IsTrue(send); // could it be true/false ?
        PLT_XmlHelper::GetChildText(stateVariables[k], "defaultValue", variable->m_DefaultValue);

        NPT_XmlElementNode* allowedValueList = PLT_XmlHelper::GetChild(stateVariables[k], "allowedValueList");
        if (allowedValueList) {
            NPT_Array<NPT_XmlElementNode*> allowedValues;
            PLT_XmlHelper::GetChildren(allowedValueList, allowedValues, "allowedValue");
            for( int l = 0 ; l < (int)allowedValues.GetItemCount(); l++) {
                const NPT_String* text = allowedValues[l]->GetText();
                if (text) {
                    variable->m_AllowedValues.Add(new NPT_String(*text));
                }
            }
        } else {
            NPT_XmlElementNode* allowedValueRange = PLT_XmlHelper::GetChild(stateVariables[k], "allowedValueRange");
            if (allowedValueRange) {
                NPT_String min, max, step;
                PLT_XmlHelper::GetChildText(allowedValueRange, "minimum", min);
                PLT_XmlHelper::GetChildText(allowedValueRange, "maximum", max);
                PLT_XmlHelper::GetChildText(allowedValueRange, "step", step);
                if (min.GetLength() == 0 || max.GetLength() == 0) {
                    goto failure;
                }
                variable->m_AllowedValueRange = new NPT_AllowedValueRange;
                NPT_ParseInteger(min, variable->m_AllowedValueRange->min_value);
                NPT_ParseInteger(max, variable->m_AllowedValueRange->max_value);
                variable->m_AllowedValueRange->step = -1;
                if (step.GetLength() != 0) {
                    NPT_ParseInteger(step, variable->m_AllowedValueRange->step);
                }
            }
        }
    }

    // actions
    if (NPT_FAILED(PLT_XmlHelper::GetChildren(actionList, actions, "action"))) {
        goto failure;
    }

    for( int i = 0 ; i < (int)actions.GetItemCount(); i++) {
        NPT_String action_name;
        PLT_XmlHelper::GetChildText(actions[i],  "name", action_name);
        if (action_name.GetLength() == 0) {
            goto failure;
        }

        PLT_ActionDesc* action_desc = new PLT_ActionDesc(action_name, this);
        m_ActionDescs.Add(action_desc);        
        
        // action arguments
        NPT_XmlElementNode* argumentList = PLT_XmlHelper::GetChild(actions[i], "argumentList");
        if (argumentList == NULL || !argumentList->GetChildren().GetItemCount())
            continue; // no arguments is ok I guess


        NPT_Array<NPT_XmlElementNode*> arguments;
        NPT_CHECK_SEVERE(PLT_XmlHelper::GetChildren(argumentList, arguments, "argument"));
        bool foundRetValue = false;
        for( int j = 0 ; j < (int)arguments.GetItemCount(); j++) {
            NPT_String name, direction, relatedStateVar;
            PLT_XmlHelper::GetChildText(arguments[j], "name", name);
            PLT_XmlHelper::GetChildText(arguments[j], "direction", direction);
            PLT_XmlHelper::GetChildText(arguments[j], "relatedStateVariable", relatedStateVar);
            if (name.GetLength() == 0 || direction.GetLength() == 0 || relatedStateVar.GetLength() == 0) {
                goto failure;
            }

            // make sure the related state variable exists
            PLT_StateVariable* variable = FindStateVariable(relatedStateVar);
            if (variable == NULL) {
                goto failure;
            }

            bool bReturnValue = false;
            NPT_XmlElementNode* retval_node = PLT_XmlHelper::GetChild(arguments[j], "retVal");
            if (retval_node) {
                // verify this is the only retVal we've had
                if (foundRetValue) {
                    goto failure;
                } else {
                    bReturnValue = true;
                    foundRetValue = true;
                }
            }
            action_desc->GetArgumentDescs().Add(new PLT_ArgumentDesc(name, direction, variable, bReturnValue));
        }
    }

    // delete the tree
    delete tree;
    return NPT_SUCCESS;

failure:
    NPT_LOG_FATAL_1("Failed to parse scpd: %s", scpd);
    delete tree;
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_Service::FindActionDesc
+---------------------------------------------------------------------*/
PLT_ActionDesc*
PLT_Service::FindActionDesc(const char* name)
{
    PLT_ActionDesc* action = NULL;
    NPT_ContainerFind(m_ActionDescs, PLT_ActionDescNameFinder(this, name), action);
    return action;
}

/*----------------------------------------------------------------------
|   PLT_Service::FindStateVariable
+---------------------------------------------------------------------*/
PLT_StateVariable*
PLT_Service::FindStateVariable(const char* name)
{
    PLT_StateVariable* stateVariable = NULL;
    NPT_ContainerFind(m_StateVars, PLT_StateVariableNameFinder(name), stateVariable);
    return stateVariable;
}

/*----------------------------------------------------------------------
|   PLT_Service::GetStateVariableValue
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::GetStateVariableValue(const char* name, NPT_String& value)
{
    PLT_StateVariable* stateVariable = FindStateVariable(name);
    NPT_CHECK_POINTER_FATAL(stateVariable);
    value = stateVariable->GetValue();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::IsSubscribable
+---------------------------------------------------------------------*/
bool
PLT_Service::IsSubscribable()
{
    NPT_List<PLT_StateVariable*>::Iterator var = m_StateVars.GetFirstItem();
    while (var) {
        if ((*var)->IsSendingEvents()) return true;
        ++var;
    }
    return false;
}

/*----------------------------------------------------------------------
|   PLT_Service::SetStateVariable
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::SetStateVariable(const char* name, const char* value)
{
    PLT_StateVariable* stateVariable = NULL;
    NPT_ContainerFind(m_StateVars, PLT_StateVariableNameFinder(name), stateVariable);
    if (stateVariable == NULL)
        return NPT_FAILURE;

    return stateVariable->SetValue(value);
}

/*----------------------------------------------------------------------
|   PLT_Service::SetStateVariableRate
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::SetStateVariableRate(const char* name, NPT_TimeInterval rate)
{
    PLT_StateVariable* stateVariable = NULL;
    NPT_ContainerFind(m_StateVars, PLT_StateVariableNameFinder(name), stateVariable);
    if (stateVariable == NULL)
        return NPT_FAILURE;

    return stateVariable->SetRate(rate);
}

/*----------------------------------------------------------------------
|   PLT_Service::IncStateVariable
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::IncStateVariable(const char* name)
{
    PLT_StateVariable* stateVariable = NULL;
    NPT_ContainerFind(m_StateVars, PLT_StateVariableNameFinder(name), stateVariable);
    if (stateVariable == NULL)
        return NPT_FAILURE;

    NPT_String value = stateVariable->GetValue();
    long num;
    if (value.GetLength() == 0 || NPT_FAILED(value.ToInteger(num))) {
        return NPT_FAILURE;
    }

    // convert value to int
    return stateVariable->SetValue(NPT_String::FromInteger(num+1));
}

/*----------------------------------------------------------------------
|   PLT_Service::ProcessNewSubscription
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::ProcessNewSubscription(PLT_TaskManager*         task_manager,
                                    const NPT_SocketAddress& addr, 
                                    const NPT_String&        callback_urls, 
                                    int                      timeout, 
                                    NPT_HttpResponse&        response)
{
    NPT_LOG_FINE_1("New subscription for %s", m_EventSubURL.GetChars());

//    // first look if we don't have a subscriber with same callbackURL
//    PLT_EventSubscriber* subscriber = NULL;
//    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Subscribers, PLT_EventSubscriberFinderByCallbackURL(strCallbackURL),
//        subscriber))) {
//        // update local interface and timeout
//        subscriber->m_local_if.SetIpAddress((unsigned long) addr.GetIpAddress());
//        subscriber->m_ExpirationTime = NPT_Time(NULL) + timeout;
//
//        PLT_UPnPMessageHelper::SetSID("uuid:" + subscriber->m_sid);
//        PLT_UPnPMessageHelper::SetTimeOut(timeout);
//        return NPT_SUCCESS;
//    }
//
    // reject if we have too many subscribers already
    if (m_Subscribers.GetItemCount() > 30) {
        response.SetStatus(500, "Internal Server Error");
        return NPT_FAILURE;
    }

    //TODO: prevent hacking by making sure callbackurl is not ourselves?

    // generate a unique subscriber ID
    NPT_String sid;
    PLT_UPnPMessageHelper::GenerateUUID(19, sid);

    PLT_EventSubscriber* subscriber = new PLT_EventSubscriber(task_manager, this, sid, timeout);
    // parse the callback URLs
    bool reachable = false;
    if (callback_urls[0] == '<') {
        char* szURLs = (char*)(const char*)callback_urls;
        char* brackL = szURLs;
        char* brackR = szURLs;
        while (++brackR < szURLs + callback_urls.GetLength()) {
            if (*brackR == '>') {
                NPT_String strCallbackURL = NPT_String(brackL+1, (NPT_Size)(brackR-brackL-1));
                NPT_HttpUrl url(strCallbackURL);

                if (url.IsValid()) {
                    subscriber->AddCallbackURL(strCallbackURL);
                    reachable = true;
                }
                brackL = ++brackR;
            }
        }
    }

    if (reachable == false) {
        NPT_CHECK_LABEL_FATAL(NPT_FAILURE, cleanup);
    }

    // keep track of which interface we receive the request, we will use this one
    // when notifying
    subscriber->SetLocalIf(addr);

    PLT_UPnPMessageHelper::SetSID(response, subscriber->GetSID());
    PLT_UPnPMessageHelper::SetTimeOut(response, timeout);

    {
        NPT_AutoLock lock(m_Lock);

        // new subscriber should get all vars in the LastChange var
        UpdateLastChange(m_StateVars);

        // send all state vars to sub
        NPT_Result res = subscriber->Notify(m_StateVars);

        // reset LastChange var to what was really just changed
        UpdateLastChange(m_StateVarsChanged);

        NPT_CHECK_LABEL_FATAL(res, cleanup);

        if (!m_EventTask) {
            m_EventTask = new PLT_ServiceEventTask(this);
            task_manager->StartTask(m_EventTask);
        }

        m_Subscribers.Add(subscriber);
    }

    return NPT_SUCCESS;

cleanup:
    response.SetStatus(412, "Precondition Failed");
    delete subscriber;
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_Service::ProcessRenewSubscription
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::ProcessRenewSubscription(const NPT_SocketAddress& addr, 
                                      const NPT_String&        sid, 
                                      int                      timeout, 
                                      NPT_HttpResponse&        response)
{
    NPT_AutoLock lock(m_Lock);

    NPT_LOG_FINE_2("Renewing subscription for %s (sub=%s)", 
        m_EventSubURL.GetChars(), 
        sid.GetChars());

    // first look if we don't have a subscriber with same callbackURL
    PLT_EventSubscriber* subscriber = NULL;
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Subscribers, 
                                        PLT_EventSubscriberFinderBySID(sid), 
                                        subscriber))) {
        // update local interface and timeout
        subscriber->SetLocalIf(addr);
        subscriber->SetTimeout(timeout);

        PLT_UPnPMessageHelper::SetSID(response, subscriber->GetSID());
        PLT_UPnPMessageHelper::SetTimeOut(response, timeout);
        return NPT_SUCCESS;
    }

    NPT_LOG_WARNING_1("Renewing subscription for unknown %s!", sid.GetChars());

    // didn't find a valid Subscriber in our list
    response.SetStatus(412, "Precondition Failed");
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_Service::ProcessCancelSubscription
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::ProcessCancelSubscription(const NPT_SocketAddress& /* addr */, 
                                       const NPT_String&        sid, 
                                       NPT_HttpResponse&        response)
{
    NPT_AutoLock lock(m_Lock);

    NPT_LOG_FINE_2("Cancelling subscription for %s (sub=%s)", 
        m_EventSubURL.GetChars(),
        sid.GetChars());

    // first look if we don't have a subscriber with same callbackURL
    PLT_EventSubscriber* sub = NULL;
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Subscribers, 
                                        PLT_EventSubscriberFinderBySID(sid), 
                                        sub))) {

        // remove sub
        m_Subscribers.Remove(sub);
        delete sub;
        return NPT_SUCCESS;
    }

    NPT_LOG_WARNING_1("Cancelling subscription for unknown %s!", sid.GetChars());

    // didn't find a valid Subscriber in our list
    response.SetStatus(412, "Precondition Failed");
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_Service::AddChanged
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::AddChanged(PLT_StateVariable* var)
{
    NPT_AutoLock lock(m_Lock);

    // no event task means no subscribers yet, so don't bother
    // Note: this will take care also when setting default state 
    //       variables values during init and avoid being published
    if (!m_EventTask) return NPT_SUCCESS;
    
    if (var->IsSendingEvents()) {
        if (!m_StateVarsToPublish.Contains(var)) 
            m_StateVarsToPublish.Add(var);
    } else if (var->IsSendingEvents(true)) {
        if (!m_StateVarsChanged.Contains(var)) 
            m_StateVarsChanged.Add(var);
        UpdateLastChange(m_StateVarsChanged);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::UpdateLastChange
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::UpdateLastChange(NPT_List<PLT_StateVariable*>& vars)
{
    PLT_StateVariable* var = FindStateVariable("LastChange");
    if (var == NULL) return NPT_FAILURE;

    NPT_ASSERT(m_LastChangeNamespace.GetLength() > 0);

    if (vars.GetItemCount() == 0) {
        // no vars to update, remove LastChange from vars to publish
        m_StateVarsToPublish.Remove(var);
        return NPT_SUCCESS;
    }

    NPT_XmlElementNode* top = new NPT_XmlElementNode("Event");
    NPT_CHECK_SEVERE(top->SetNamespaceUri("", m_LastChangeNamespace));

    NPT_XmlElementNode* instance = new NPT_XmlElementNode("InstanceID");
    NPT_CHECK_SEVERE(top->AddChild(instance));
    NPT_CHECK_SEVERE(instance->SetAttribute("val", "0"));

    // build list of changes
    NPT_CHECK_SEVERE(vars.Apply(PLT_LastChangeXMLIterator(instance)));

    // serialize node
    NPT_String value;
    NPT_CHECK_SEVERE(PLT_XmlHelper::Serialize(*top, value));
    delete top;

    // set the state change direcly instead of calling SetValue
    // to avoid recursive lock, instead add var to publish here directly
    var->m_Value = value;
    if (!m_StateVarsToPublish.Contains(var)) m_StateVarsToPublish.Add(var);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::PauseEventing
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::PauseEventing(bool paused)
{
    NPT_AutoLock lock(m_Lock);
    m_EventingPaused = paused;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_Service::NotifyChanged
+---------------------------------------------------------------------*/
NPT_Result
PLT_Service::NotifyChanged()
{
    NPT_AutoLock lock(m_Lock);

    // no eventing for now
    if (m_EventingPaused) return NPT_SUCCESS;

    // pick the vars that are ready to be published
    // based on their moderation rate and last publication
    NPT_List<PLT_StateVariable*> vars_ready;
    NPT_List<PLT_StateVariable*>::Iterator iter = m_StateVarsToPublish.GetFirstItem();
    while (iter) {
        PLT_StateVariable* var = *iter;
        if (var->IsReadyToPublish()) {
            vars_ready.Add(var);
            m_StateVarsToPublish.Erase(iter++);

            // clear last changed list if we're about to send LastChange var
            if (!var->GetName().Compare("LastChange")) m_StateVarsChanged.Clear();
        } else {
            iter++;
        }
    }
    
    // send vars that are ready to go and remove old subscribers
    int i = 0;
    int count = m_Subscribers.GetItemCount();
    while (i++ < count) {
        PLT_EventSubscriber* sub;
        if (NPT_SUCCEEDED(m_Subscribers.PopHead(sub))) {
            NPT_TimeStamp now, expiration;
            NPT_System::GetCurrentTimeStamp(now);
            expiration = sub->GetExpirationTime();

            // forget sub if it didn't renew in time or if notification failed
            if (expiration == NPT_TimeStamp() || expiration > now ) {
                NPT_Result res = vars_ready.GetItemCount()?sub->Notify(vars_ready):NPT_SUCCESS;
                if (NPT_SUCCEEDED(res)) {
                    m_Subscribers.Add(sub);
                    continue;
                }
            }
            
            delete sub;
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_ServiceSCPDURLFinder::operator()
+---------------------------------------------------------------------*/
bool 
PLT_ServiceSCPDURLFinder::operator()(PLT_Service* const & service) const 
{
    NPT_String url = service->GetSCPDURL();
    if (!url.StartsWith("/")) {
        url = service->GetDevice()->GetURLBase().GetPath() + url;
    }
    return m_URL.Compare(url, true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_ServiceControlURLFinder::operator()
+---------------------------------------------------------------------*/
bool 
PLT_ServiceControlURLFinder::operator()(PLT_Service* const & service) const 
{
    NPT_String url = service->GetControlURL();
    if (!url.StartsWith("/")) {
        url = service->GetDevice()->GetURLBase().GetPath() + url;
    }
    return m_URL.Compare(url, true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_ServiceEventSubURLFinder::operator()
+---------------------------------------------------------------------*/
bool
PLT_ServiceEventSubURLFinder::operator()(PLT_Service* const & service) const 
{
    NPT_String url = service->GetEventSubURL();
    if (!url.StartsWith("/")) {
        url = service->GetDevice()->GetURLBase().GetPath() + url;
    }
    return m_URL.Compare(url, true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_ServiceIDFinder::operator()
+---------------------------------------------------------------------*/
bool
PLT_ServiceIDFinder::operator()(PLT_Service* const & service) const 
{
    return m_Id.Compare(service->GetServiceID(), true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_ServiceTypeFinder::operator()
+---------------------------------------------------------------------*/
bool 
PLT_ServiceTypeFinder::operator()(PLT_Service* const & service) const 
{
    return m_Type.Compare(service->GetServiceType(), true) ? false : true;
}

/*----------------------------------------------------------------------
|   PLT_GetLastChangeXMLIterator::operator()
+---------------------------------------------------------------------*/
NPT_Result
PLT_LastChangeXMLIterator::operator()(PLT_StateVariable* const &var) const
{   
    // only add vars that are indirectly evented
    if (!var->IsSendingEvents(true)) return NPT_SUCCESS;

    NPT_XmlElementNode* variable = new NPT_XmlElementNode((const char*)var->GetName());
    NPT_CHECK_SEVERE(m_Node->AddChild(variable));
    NPT_CHECK_SEVERE(variable->SetAttribute("val", var->GetValue()));
    return NPT_SUCCESS;
}
