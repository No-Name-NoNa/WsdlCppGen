package config

import "testing"

func TestLoadUsesDefaults(t *testing.T) {
	cfg, err := Load(nil, func(string) string { return "" })
	if err != nil {
		t.Fatal(err)
	}

	if cfg.Listen != DefaultListen ||
		cfg.BaseURL != DefaultBaseURL ||
		cfg.ServicePath != DefaultServicePath ||
		cfg.Namespace != DefaultNamespace ||
		cfg.WSDLTemplate != DefaultWSDLTemplate {
		t.Fatalf("unexpected defaults: %+v", cfg)
	}
	if cfg.Endpoint() != "http://localhost:8080/services/AddService" {
		t.Fatalf("unexpected endpoint: %s", cfg.Endpoint())
	}
}

func TestLoadUsesEnv(t *testing.T) {
	values := map[string]string{
		"ADDSERVICE_LISTEN":        ":9090",
		"ADDSERVICE_BASE_URL":      "http://localhost:9090/",
		"ADDSERVICE_SERVICE_PATH":  "soap/AddService",
		"ADDSERVICE_NAMESPACE":     "urn:addservice",
		"ADDSERVICE_WSDL_TEMPLATE": "custom.wsdl.tmpl",
	}

	cfg, err := Load(nil, func(name string) string { return values[name] })
	if err != nil {
		t.Fatal(err)
	}

	if cfg.Listen != ":9090" ||
		cfg.Endpoint() != "http://localhost:9090/soap/AddService" ||
		cfg.Namespace != "urn:addservice" ||
		cfg.WSDLTemplate != "custom.wsdl.tmpl" {
		t.Fatalf("env values were not applied: %+v", cfg)
	}
}

func TestLoadFlagsOverrideEnv(t *testing.T) {
	cfg, err := Load(
		[]string{
			"--listen", ":7070",
			"--base-url", "http://127.0.0.1:7070",
			"--service-path", "/custom",
			"--namespace", "urn:flag",
			"--wsdl-template", "flag.wsdl.tmpl",
		},
		func(name string) string {
			if name == "ADDSERVICE_LISTEN" {
				return ":9090"
			}
			return ""
		},
	)
	if err != nil {
		t.Fatal(err)
	}

	if cfg.Listen != ":7070" ||
		cfg.Endpoint() != "http://127.0.0.1:7070/custom" ||
		cfg.Namespace != "urn:flag" ||
		cfg.WSDLTemplate != "flag.wsdl.tmpl" {
		t.Fatalf("flags did not override env/defaults: %+v", cfg)
	}
}
