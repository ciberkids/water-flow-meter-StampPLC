describe("Translation exporter parity", () => {
  it("generates firmware assets consistent with the mockup dataset", () => {
    cy.request("POST", "/api/export").its("status").should("eq", 200);
    cy.task<{ mismatches?: string[]; error?: string }>("compareExportedDataset").then((result) => {
      expect(result.error, "export comparison error").to.be.undefined;
      expect(result.mismatches, "dataset vs IR mismatches").to.deep.equal([]);
    });
  });
});
