// Copyright 2021 The Tint Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// check-spec-examples tests that WGSL specification examples compile as
// expected.
//
// The tool parses the WGSL HTML specification from the web or from a local file
// and then runs the WGSL compiler for all examples annotated with the 'wgsl'
// and 'global-scope' or 'function-scope' HTML class types.
//
// To run:
//   go get golang.org/x/net/html # Only required once
//   go run tools/check-spec-examples/main.go --compiler=<path-to-tint>
package main

import (
	"errors"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	"golang.org/x/net/html"
)

const (
	toolName        = "check-spec-examples"
	defaultSpecPath = "https://gpuweb.github.io/gpuweb/wgsl.html"
)

var (
	errInvalidArg = errors.New("Invalid arguments")
)

func main() {
	flag.Usage = func() {
		out := flag.CommandLine.Output()
		fmt.Fprintf(out, "%v tests that WGSL specification examples compile as expected.\n", toolName)
		fmt.Fprintf(out, "\n")
		fmt.Fprintf(out, "Usage:\n")
		fmt.Fprintf(out, "  %s [spec] [flags]\n", toolName)
		fmt.Fprintf(out, "\n")
		fmt.Fprintf(out, "spec is an optional local file path or URL to the WGSL specification.\n")
		fmt.Fprintf(out, "If spec is omitted then the specification is fetched from %v\n", defaultSpecPath)
		fmt.Fprintf(out, "\n")
		fmt.Fprintf(out, "flags may be any combination of:\n")
		flag.PrintDefaults()
	}

	err := run()
	switch err {
	case nil:
		return
	case errInvalidArg:
		fmt.Fprintf(os.Stderr, "Error: %v\n\n", err)
		flag.Usage()
	default:
		fmt.Fprintf(os.Stderr, "%v\n", err)
	}
	os.Exit(1)
}

func run() error {
	// Parse flags
	compilerPath := flag.String("compiler", "tint", "path to compiler executable")
	verbose := flag.Bool("verbose", false, "print examples that pass")
	flag.Parse()

	// Try to find the WGSL compiler
	compiler, err := exec.LookPath(*compilerPath)
	if err != nil {
		return fmt.Errorf("Failed to find WGSL compiler: %w", err)
	}
	if compiler, err = filepath.Abs(compiler); err != nil {
		return fmt.Errorf("Failed to find WGSL compiler: %w", err)
	}

	// Check for explicit WGSL spec path
	args := flag.Args()
	specURL, _ := url.Parse(defaultSpecPath)
	switch len(args) {
	case 0:
	case 1:
		var err error
		specURL, err = url.Parse(args[0])
		if err != nil {
			return err
		}
	default:
		if len(args) > 1 {
			return errInvalidArg
		}
	}

	// The specURL might just be a local file path, in which case automatically
	// add the 'file' URL scheme
	if specURL.Scheme == "" {
		specURL.Scheme = "file"
	}

	// Open the spec from HTTP(S) or from a local file
	var specContent io.ReadCloser
	switch specURL.Scheme {
	case "http", "https":
		response, err := http.Get(specURL.String())
		if err != nil {
			return fmt.Errorf("Failed to load the WGSL spec from '%v': %w", specURL, err)
		}
		specContent = response.Body
	case "file":
		specURL.Path, err = filepath.Abs(specURL.Path)
		if err != nil {
			return fmt.Errorf("Failed to load the WGSL spec from '%v': %w", specURL, err)
		}

		file, err := os.Open(specURL.Path)
		if err != nil {
			return fmt.Errorf("Failed to load the WGSL spec from '%v': %w", specURL, err)
		}
		specContent = file
	default:
		return fmt.Errorf("Unsupported URL scheme: %v", specURL.Scheme)
	}
	defer specContent.Close()

	// Create the HTML parser
	doc, err := html.Parse(specContent)
	if err != nil {
		return err
	}

	// Parse all the WGSL examples
	examples := []example{}
	if err := gatherExamples(doc, &examples); err != nil {
		return err
	}

	// Create a temporary directory to hold the examples as separate files
	tmpDir, err := ioutil.TempDir("", "wgsl-spec-examples")
	if err != nil {
		return err
	}
	if err := os.MkdirAll(tmpDir, 0666); err != nil {
		return fmt.Errorf("Failed to create temporary directory: %w", err)
	}
	defer os.RemoveAll(tmpDir)

	// For each compilable WGSL example...
	for _, e := range examples {
		exampleURL := specURL.String() + "#" + e.name

		if err := tryCompile(compiler, tmpDir, e); err != nil {
			if !e.expectError {
				fmt.Printf("✘ %v ✘\n%v\n", exampleURL, err)
				continue
			}
		} else if e.expectError {
			fmt.Printf("✘ %v ✘\nCompiled even though it was marked with 'expect-error'\n", exampleURL)
		}
		if *verbose {
			fmt.Printf("✔ %v ✔\n", exampleURL)
		}
	}
	return nil
}

// Holds all the information about a single, compilable WGSL example in the spec
type example struct {
	name          string // The name (typically hash generated by bikeshed)
	code          string // The example source
	globalScope   bool   // Annotated with 'global-scope' ?
	functionScope bool   // Annotated with 'function-scope' ?
	expectError   bool   // Annotated with 'expect-error' ?
}

// tryCompile attempts to compile the example e in the directory wd, using the
// compiler at the given path. If the example is annotated with 'function-scope'
// then the code is wrapped with a basic vertex-stage-entry function.
// If the first compile fails with an error message containing 'error v-0003',
// then a dummy vertex-state-entry function is appended to the source, and
// another attempt to compile the shader is made.
func tryCompile(compiler, wd string, e example) error {
	code := e.code
	if e.functionScope {
		code = "\n[[stage(vertex)]] fn main() {\n" + code + "}\n"
	}

	addedStubFunction := false
	for {
		err := compile(compiler, wd, e.name, code)
		if err == nil {
			return nil
		}

		if !addedStubFunction && strings.Contains(err.Error(), "error v-0003") {
			// error v-0003: At least one of vertex, fragment or compute shader
			// must be present. Add a stub entry point to satisfy the compiler.
			code += "\n[[stage(vertex)]] fn main() {}\n"
			addedStubFunction = true
			continue
		}

		return err
	}
}

// compile creates a file in wd and uses the compiler to attempt to compile it.
func compile(compiler, wd, name, code string) error {
	filename := name + ".wgsl"
	path := filepath.Join(wd, filename)
	if err := ioutil.WriteFile(path, []byte(code), 0666); err != nil {
		return fmt.Errorf("Failed to write example file '%v'", path)
	}
	cmd := exec.Command(compiler, filename)
	cmd.Dir = wd
	out, err := cmd.CombinedOutput()
	if err != nil {
		return fmt.Errorf("%v\n%v", err, string(out))
	}
	return nil
}

// gatherExamples scans the HTML node and its children for blocks that contain
// WGSL example code, populating the examples slice.
func gatherExamples(node *html.Node, examples *[]example) error {
	if hasClass(node, "example") && hasClass(node, "wgsl") {
		e := example{
			name:          nodeID(node),
			code:          exampleCode(node),
			globalScope:   hasClass(node, "global-scope"),
			functionScope: hasClass(node, "function-scope"),
			expectError:   hasClass(node, "expect-error"),
		}
		// If the example is annotated with a scope, then it can be compiled.
		if e.globalScope || e.functionScope {
			*examples = append(*examples, e)
		}
	}
	for child := node.FirstChild; child != nil; child = child.NextSibling {
		if err := gatherExamples(child, examples); err != nil {
			return err
		}
	}
	return nil
}

// exampleCode returns a string formed from all the TextNodes found in <pre>
// blocks that are children of node.
func exampleCode(node *html.Node) string {
	sb := strings.Builder{}
	for child := node.FirstChild; child != nil; child = child.NextSibling {
		if child.Data == "pre" {
			printNodeText(child, &sb)
		}
	}
	return sb.String()
}

// printNodeText traverses node and its children, writing the Data of all
// TextNodes to sb.
func printNodeText(node *html.Node, sb *strings.Builder) {
	if node.Type == html.TextNode {
		sb.WriteString(node.Data)
	}

	for child := node.FirstChild; child != nil; child = child.NextSibling {
		printNodeText(child, sb)
	}
}

// hasClass returns true iff node is has the given "class" attribute.
func hasClass(node *html.Node, class string) bool {
	for _, attr := range node.Attr {
		if attr.Key == "class" {
			classes := strings.Split(attr.Val, " ")
			for _, c := range classes {
				if c == class {
					return true
				}
			}
		}
	}
	return false
}

// nodeID returns the given "id" attribute of node, or an empty string if there
// is no "id" attribute.
func nodeID(node *html.Node) string {
	for _, attr := range node.Attr {
		if attr.Key == "id" {
			return attr.Val
		}
	}
	return ""
}
