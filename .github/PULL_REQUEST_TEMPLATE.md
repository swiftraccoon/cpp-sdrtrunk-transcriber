# Pull Request

## Summary

Brief description of the changes in this PR.

## Type of Change

- [ ] 🐛 Bug fix (non-breaking change that fixes an issue)
- [ ] ✨ New feature (non-breaking change that adds functionality)
- [ ] 🔥 Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] 📄 Documentation update
- [ ] 🧪 Refactoring (no functional changes)
- [ ] ⚙️ Build/CI changes
- [ ] ✅ Tests (adding or updating tests)

## Related Issues

<!-- Link to related issues using #issue_number -->
Fixes #(issue_number)
Related to #(issue_number)

## Changes Made

### Core Changes
- List major changes made to the codebase
- Include new files added, modified, or removed
- Describe architectural or design changes

### Configuration Changes
- [ ] Configuration file format changes
- [ ] New configuration options added
- [ ] Command-line interface changes
- [ ] Database schema changes

### Documentation Updates
- [ ] README.md updated
- [ ] API documentation updated
- [ ] Build instructions updated
- [ ] Code comments added/improved

## Testing

### Test Environment
- **OS**: [e.g., Ubuntu 22.04, Windows 11]
- **Compiler**: [e.g., GCC 11.2, MSVC 2022]
- **Build Type**: [Debug/Release]
- **Dependencies**: [Any specific versions or configurations]

### Tests Performed
- [ ] Unit tests pass (`ctest`)
- [ ] Manual testing with sample files
- [ ] Configuration validation
- [ ] Memory leak testing (valgrind/AddressSanitizer)
- [ ] Performance impact assessment

### Test Cases
1. **Test case 1**: Description of what was tested and results
2. **Test case 2**: Description of what was tested and results
3. **Edge cases**: Any edge cases or error conditions tested

## Code Quality

- [ ] Code follows project style guidelines
- [ ] Code has been formatted with clang-format
- [ ] Static analysis passes (clang-tidy, cppcheck)
- [ ] No compiler warnings in Release build
- [ ] Memory safety verified (no leaks, proper RAII)
- [ ] Error handling properly implemented

## Performance Impact

- [ ] No performance regression
- [ ] Performance improvement (describe)
- [ ] Performance impact assessed and acceptable
- [ ] Benchmark results (if applicable)

## Backward Compatibility

- [ ] Fully backward compatible
- [ ] Breaking changes documented
- [ ] Migration guide provided (if needed)
- [ ] Configuration file compatibility maintained
- [ ] Database schema migration handled

## Security Considerations

- [ ] No security vulnerabilities introduced
- [ ] Input validation properly implemented
- [ ] Sensitive data handling reviewed
- [ ] Dependencies security reviewed

## Deployment Notes

**Installation/Upgrade Instructions:**
- Any special steps needed for deployment
- Configuration changes required
- Database migrations needed

**Rollback Plan:**
- How to revert these changes if needed
- Any data that would be lost in rollback

## Screenshots/Examples

<!-- If applicable, add screenshots or example output -->

**Before:**
```
# Example of behavior before changes
```

**After:**
```
# Example of behavior after changes
```

## Checklist

### Development
- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] Code coverage maintained/improved
- [ ] Documentation updated
- [ ] CHANGELOG.md updated

### Review Ready
- [ ] Self-review completed
- [ ] Complex logic documented
- [ ] Ready for maintainer review
- [ ] CI/CD pipeline passes

### Pre-Merge
- [ ] Final testing in target environment
- [ ] All review comments addressed
- [ ] Squashed commits if requested
- [ ] Branch up to date with main

## Additional Notes

<!-- Any additional information that reviewers should know -->

### Future Work
- Items that could be improved in future PRs
- Known limitations of the current implementation
- Suggestions for follow-up enhancements

### Review Focus Areas
- Specific areas where you'd like reviewer attention
- Parts of the code you're uncertain about
- Performance-critical sections

---

**For Reviewers:**
- Please test with your local environment if possible
- Pay special attention to error handling and edge cases
- Verify documentation accuracy
- Check for potential security or performance issues