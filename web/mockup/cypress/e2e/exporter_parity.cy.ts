describe("Translation exporter parity", () => {
  it("generates firmware assets consistent with the mockup dataset", () => {
    cy.task<{ ok?: boolean; error?: string }>("runExportWithFixture").then((result) => {
      expect(result.error, "export build error").to.be.undefined;
      expect(result.ok, "export completed").to.equal(true);
    });
    cy.task<{ mismatches?: string[]; error?: string }>("compareExportedDataset").then((result) => {
      expect(result.error, "export comparison error").to.be.undefined;
      expect(result.mismatches, "dataset vs IR mismatches").to.deep.equal([]);
    });
  });
});
