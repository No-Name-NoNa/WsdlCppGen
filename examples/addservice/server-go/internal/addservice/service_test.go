package addservice

import (
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestHandlerAddsNumbers(t *testing.T) {
	body := `<?xml version="1.0" encoding="utf-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" xmlns:tns="http://example.com/addservice">
  <soap:Body>
    <tns:AddRequest>
      <num1>3</num1>
      <num2>5</num2>
    </tns:AddRequest>
  </soap:Body>
</soap:Envelope>`

	handler := NewHandler(HandlerConfig{
		Namespace: "http://example.com/addservice",
		WSDL:      "wsdl document",
	})
	request := httptest.NewRequest(http.MethodPost, "/services/AddService", strings.NewReader(body))
	response := httptest.NewRecorder()

	handler.ServeHTTP(response, request)

	if response.Code != http.StatusOK {
		t.Fatalf("unexpected status: %d", response.Code)
	}
	if !strings.Contains(response.Body.String(), "<result>8</result>") {
		t.Fatalf("response does not contain result 8: %s", response.Body.String())
	}
}

func TestHandlerReturnsWSDL(t *testing.T) {
	handler := NewHandler(HandlerConfig{
		Namespace: "http://example.com/addservice",
		WSDL:      "AddService WSDL",
	})
	request := httptest.NewRequest(http.MethodGet, "/services/AddService?wsdl", nil)
	response := httptest.NewRecorder()

	handler.ServeHTTP(response, request)

	if response.Code != http.StatusOK {
		t.Fatalf("unexpected status: %d", response.Code)
	}
	if !strings.Contains(response.Body.String(), "AddService WSDL") {
		t.Fatalf("WSDL response did not use configured document")
	}
}

func TestHandlerReturnsFaultForInvalidXML(t *testing.T) {
	handler := NewHandler(HandlerConfig{
		Namespace: "http://example.com/addservice",
		WSDL:      "wsdl document",
	})
	request := httptest.NewRequest(http.MethodPost, "/services/AddService", strings.NewReader("<not-closed"))
	response := httptest.NewRecorder()

	handler.ServeHTTP(response, request)

	if response.Code != http.StatusBadRequest {
		t.Fatalf("unexpected status: %d", response.Code)
	}
	if !strings.Contains(response.Body.String(), "<soap:Fault>") {
		t.Fatalf("response does not contain SOAP fault: %s", response.Body.String())
	}
}

func TestRenderWSDLUsesTemplateData(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "AddService.wsdl.tmpl")
	template := `namespace={{.TargetNamespace}} endpoint={{.Endpoint}} action={{.SoapAction}}`
	if err := os.WriteFile(path, []byte(template), 0o600); err != nil {
		t.Fatal(err)
	}

	wsdl, err := RenderWSDL(path, WSDLData{
		TargetNamespace: "urn:test",
		Endpoint:        "http://localhost:9090/soap",
		SoapAction:      "urn:test/Add",
	})
	if err != nil {
		t.Fatal(err)
	}
	if !strings.Contains(wsdl, "namespace=urn:test") ||
		!strings.Contains(wsdl, "endpoint=http://localhost:9090/soap") ||
		!strings.Contains(wsdl, "action=urn:test/Add") {
		t.Fatalf("rendered WSDL did not include template data: %s", wsdl)
	}
}
