package main

import (
	"encoding/xml"
	"fmt"
	"io"
	"log"
	"net/http"
	"strconv"
	"strings"
)

const wsdlDocument = `<?xml version="1.0" encoding="UTF-8"?>
<wsdl:definitions
    xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/"
    xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/"
    xmlns:xsd="http://www.w3.org/2001/XMLSchema"
    xmlns:tns="http://example.com/addservice"
    targetNamespace="http://example.com/addservice">
    <wsdl:types>
        <xsd:schema targetNamespace="http://example.com/addservice">
            <xsd:element name="AddRequest">
                <xsd:complexType>
                    <xsd:sequence>
                        <xsd:element name="num1" type="xsd:int"/>
                        <xsd:element name="num2" type="xsd:int"/>
                    </xsd:sequence>
                </xsd:complexType>
            </xsd:element>
            <xsd:element name="AddResponse">
                <xsd:complexType>
                    <xsd:sequence>
                        <xsd:element name="result" type="xsd:int"/>
                    </xsd:sequence>
                </xsd:complexType>
            </xsd:element>
        </xsd:schema>
    </wsdl:types>
    <wsdl:message name="AddInput">
        <wsdl:part name="input" element="tns:AddRequest"/>
    </wsdl:message>
    <wsdl:message name="AddOutput">
        <wsdl:part name="output" element="tns:AddResponse"/>
    </wsdl:message>
    <wsdl:portType name="AddPortType">
        <wsdl:operation name="Add">
            <wsdl:input message="tns:AddInput"/>
            <wsdl:output message="tns:AddOutput"/>
        </wsdl:operation>
    </wsdl:portType>
    <wsdl:binding name="AddSoapBinding" type="tns:AddPortType">
        <soap:binding style="document" transport="http://schemas.xmlsoap.org/soap/http"/>
        <wsdl:operation name="Add">
            <soap:operation soapAction="http://example.com/addservice/Add" style="document"/>
            <wsdl:input>
                <soap:body use="literal"/>
            </wsdl:input>
            <wsdl:output>
                <soap:body use="literal"/>
            </wsdl:output>
        </wsdl:operation>
    </wsdl:binding>
    <wsdl:service name="AddService">
        <wsdl:port name="AddPort" binding="tns:AddSoapBinding">
            <soap:address location="http://localhost:8080/services/AddService"/>
        </wsdl:port>
    </wsdl:service>
</wsdl:definitions>`

type addRequest struct {
	Num1 int
	Num2 int
}

func parseAddRequest(body io.Reader) (addRequest, error) {
	decoder := xml.NewDecoder(body)
	var request addRequest
	for {
		token, err := decoder.Token()
		if err == io.EOF {
			break
		}
		if err != nil {
			return request, err
		}
		start, ok := token.(xml.StartElement)
		if !ok {
			continue
		}
		var value string
		switch start.Name.Local {
		case "num1":
			if err := decoder.DecodeElement(&value, &start); err != nil {
				return request, err
			}
			parsed, err := strconv.Atoi(strings.TrimSpace(value))
			if err != nil {
				return request, err
			}
			request.Num1 = parsed
		case "num2":
			if err := decoder.DecodeElement(&value, &start); err != nil {
				return request, err
			}
			parsed, err := strconv.Atoi(strings.TrimSpace(value))
			if err != nil {
				return request, err
			}
			request.Num2 = parsed
		}
	}
	return request, nil
}

func soapResponse(result int) string {
	return fmt.Sprintf(`<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <AddResponse xmlns="http://example.com/addservice">
      <result>%d</result>
    </AddResponse>
  </soap:Body>
</soap:Envelope>`, result)
}

func soapFault(message string) string {
	return fmt.Sprintf(`<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <soap:Fault>
      <faultcode>soap:Client</faultcode>
      <faultstring>%s</faultstring>
    </soap:Fault>
  </soap:Body>
</soap:Envelope>`, message)
}

func addServiceHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet && r.URL.RawQuery == "wsdl" {
		w.Header().Set("Content-Type", "text/xml; charset=utf-8")
		_, _ = w.Write([]byte(wsdlDocument))
		return
	}
	if r.Method != http.MethodPost {
		http.Error(w, "use POST for SOAP calls or GET ?wsdl for the WSDL", http.StatusMethodNotAllowed)
		return
	}

	request, err := parseAddRequest(r.Body)
	if err != nil {
		w.Header().Set("Content-Type", "text/xml; charset=utf-8")
		w.WriteHeader(http.StatusBadRequest)
		_, _ = w.Write([]byte(soapFault(err.Error())))
		return
	}

	w.Header().Set("Content-Type", "text/xml; charset=utf-8")
	_, _ = w.Write([]byte(soapResponse(request.Num1 + request.Num2)))
}

func main() {
	http.HandleFunc("/services/AddService", addServiceHandler)
	log.Println("AddService listening on http://localhost:8080/services/AddService")
	log.Println("WSDL available at http://localhost:8080/services/AddService?wsdl")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
