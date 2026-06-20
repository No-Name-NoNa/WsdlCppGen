package addservice

import (
	"bytes"
	"encoding/xml"
	"fmt"
	"html"
	"io"
	"net/http"
	"strconv"
	"strings"
	"text/template"
)

const contentTypeXML = "text/xml; charset=utf-8"

type WSDLData struct {
	TargetNamespace string
	Endpoint        string
	SoapAction      string
}

type HandlerConfig struct {
	Namespace string
	WSDL      string
}

type Handler struct {
	namespace string
	wsdl      string
}

type addRequest struct {
	Num1 int
	Num2 int
}

func RenderWSDL(templatePath string, data WSDLData) (string, error) {
	tmpl, err := template.ParseFiles(templatePath)
	if err != nil {
		return "", err
	}

	var out bytes.Buffer
	if err := tmpl.Execute(&out, data); err != nil {
		return "", err
	}
	return out.String(), nil
}

func NewHandler(cfg HandlerConfig) *Handler {
	return &Handler{
		namespace: cfg.Namespace,
		wsdl:      cfg.WSDL,
	}
}

func (h *Handler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet && hasWSDLQuery(r) {
		w.Header().Set("Content-Type", contentTypeXML)
		_, _ = w.Write([]byte(h.wsdl))
		return
	}
	if r.Method != http.MethodPost {
		http.Error(w, "use POST for SOAP calls or GET ?wsdl for the WSDL", http.StatusMethodNotAllowed)
		return
	}

	request, err := parseAddRequest(r.Body)
	if err != nil {
		w.Header().Set("Content-Type", contentTypeXML)
		w.WriteHeader(http.StatusBadRequest)
		_, _ = w.Write([]byte(soapFault(err.Error())))
		return
	}

	w.Header().Set("Content-Type", contentTypeXML)
	_, _ = w.Write([]byte(soapResponse(h.namespace, request.Num1+request.Num2)))
}

func hasWSDLQuery(r *http.Request) bool {
	_, exists := r.URL.Query()["wsdl"]
	return exists
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

func soapResponse(namespace string, result int) string {
	return fmt.Sprintf(`<?xml version="1.0" encoding="UTF-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <AddResponse xmlns="%s">
      <result>%d</result>
    </AddResponse>
  </soap:Body>
</soap:Envelope>`, html.EscapeString(namespace), result)
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
</soap:Envelope>`, html.EscapeString(message))
}
